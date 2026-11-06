// ledblink - blink an LED on the STM32MP157D-DK1 through the Linux LED class (sysfs).
//
// Usage: ledblink [period_ms] [count] [led]
//   period_ms  time the LED stays on, then off (default 500)
//   count      number of blinks, 0 = forever (default 10)
//   led        name under /sys/class/leds (default "heartbeat" = blue LD8)
//
// The kernel exposes each LED as a folder of files:
//   trigger     - what drives the LED automatically ("heartbeat", "none", ...)
//   brightness  - 0 = off, anything else = on (only sticks when trigger is "none")
// We switch the trigger off, toggle brightness ourselves, then restore the trigger.

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LED_ROOT "/sys/class/leds"

static volatile sig_atomic_t stop;

static void on_signal(int sig)
{
	(void)sig;
	stop = 1;
}

// Write a string to /sys/class/leds/<led>/<attr>.
static int led_write(const char *led, const char *attr, const char *value)
{
	char path[256];
	snprintf(path, sizeof(path), LED_ROOT "/%s/%s", led, attr);

	FILE *f = fopen(path, "w");
	if (!f) {
		fprintf(stderr, "ledblink: cannot open %s: %s\n", path, strerror(errno));
		return -1;
	}
	int ok = fputs(value, f) >= 0;
	ok = (fclose(f) == 0) && ok;	// sysfs reports write errors on close
	if (!ok)
		fprintf(stderr, "ledblink: cannot write '%s' to %s: %s\n", value, path, strerror(errno));
	return ok ? 0 : -1;
}

// Read the active trigger. The file lists all triggers with the active one
// in brackets, e.g. "none mmc0 [heartbeat] timer", so we extract that word.
static int led_get_trigger(const char *led, char *out, size_t len)
{
	char path[256], buf[4096];
	snprintf(path, sizeof(path), LED_ROOT "/%s/trigger", led);

	FILE *f = fopen(path, "r");
	if (!f) {
		fprintf(stderr, "ledblink: cannot open %s: %s\n", path, strerror(errno));
		return -1;
	}
	size_t n = fread(buf, 1, sizeof(buf) - 1, f);
	fclose(f);
	buf[n] = '\0';

	char *start = strchr(buf, '[');
	char *end = start ? strchr(start, ']') : NULL;
	if (!start || !end) {
		snprintf(out, len, "none");
		return 0;
	}
	*end = '\0';
	snprintf(out, len, "%s", start + 1);
	return 0;
}

static void sleep_ms(long ms)
{
	struct timespec ts = { .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1000000L };
	nanosleep(&ts, NULL);	// returns early on Ctrl+C, which is what we want
}

int main(int argc, char **argv)
{
	long period_ms = argc > 1 ? strtol(argv[1], NULL, 10) : 500;
	long count = argc > 2 ? strtol(argv[2], NULL, 10) : 10;
	const char *led = argc > 3 ? argv[3] : "heartbeat";

	if (period_ms <= 0 || count < 0) {
		fprintf(stderr, "usage: %s [period_ms>0] [count>=0, 0=forever] [led]\n", argv[0]);
		return 2;
	}

	char saved_trigger[64];
	if (led_get_trigger(led, saved_trigger, sizeof(saved_trigger)) < 0)
		return 1;

	// Restore the LED even when stopped with Ctrl+C or `kill`.
	struct sigaction sa = { .sa_handler = on_signal };
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

	if (led_write(led, "trigger", "none") < 0)
		return 1;

	printf("Blinking '%s' every %ld ms, %s (was trigger '%s'). Ctrl+C to stop.\n",
	       led, period_ms, count ? "limited" : "forever", saved_trigger);
	if (count)
		printf("Count: %ld\n", count);

	for (long i = 0; !stop && (count == 0 || i < count); i++) {
		led_write(led, "brightness", "1");
		sleep_ms(period_ms);
		if (stop)
			break;
		led_write(led, "brightness", "0");
		sleep_ms(period_ms);
	}

	led_write(led, "brightness", "0");
	led_write(led, "trigger", saved_trigger);
	printf("Done, restored trigger '%s'.\n", saved_trigger);
	return 0;
}
