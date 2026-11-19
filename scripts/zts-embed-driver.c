/* Threaded embed-SAPI driver for scripts/zts-embed-gshutdown.sh.
 *
 * Renders one chart on the main thread, then on THREADS worker threads
 * (REQS requests each). With FREE_THREAD=0 the workers exit without
 * ts_free_thread(), so module shutdown runs each worker's GSHUTDOWN from
 * the main thread through ts_free_id(). */
#include <sapi/embed/php_embed.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static int reqs = 4;
static int free_thread = 1;
static const char *script;

static void *worker(void *arg)
{
	(void)arg;
	(void)ts_resource(0);
	for (int r = 0; r < reqs; r++) {
		if (php_request_startup() == FAILURE) {
			fprintf(stderr, "request startup failed\n");
			exit(2);
		}
		zend_first_try {
			zend_eval_string((char *)script, NULL, "worker");
		} zend_end_try();
		php_request_shutdown(NULL);
	}
	if (free_thread) {
		ts_free_thread();
	}
	return NULL;
}

int main(int argc, char **argv)
{
	if (argc < 6) {
		fprintf(stderr, "usage: %s EXTENSION_SO THREADS REQS FREE_THREAD SCRIPT\n",
			argv[0]);
		return 1;
	}
	int threads = atoi(argv[2]);
	reqs = atoi(argv[3]);
	free_thread = atoi(argv[4]);
	script = argv[5];
	if (threads < 1 || reqs < 1) {
		return 1;
	}

	static char inipath[4096];
	snprintf(inipath, sizeof inipath, "%s.ini", argv[0]);
	FILE *f = fopen(inipath, "w");
	if (!f) {
		return 4;
	}
	fprintf(f, "extension=%s\n", argv[1]);
	fclose(f);
	php_embed_module.php_ini_path_override = inipath;

	char *eargv[] = {"driver", NULL};
	if (php_embed_init(1, eargv) == FAILURE) {
		return 3;
	}
	zend_eval_string((char *)script, NULL, "main");

	pthread_t *t = calloc((size_t)threads, sizeof *t);
	if (!t) {
		return 5;
	}
	for (int i = 0; i < threads; i++) {
		pthread_create(&t[i], NULL, worker, NULL);
	}
	for (int i = 0; i < threads; i++) {
		pthread_join(t[i], NULL);
	}
	free(t);
	php_embed_shutdown();
	fprintf(stderr, "driver done threads=%d reqs=%d free_thread=%d\n",
		threads, reqs, free_thread);
	return 0;
}
