/* SPDX-License-Identifier: ISC */

#include <err.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int run(char *cmd[])
{
	pid_t pid;
	int rc;

	pid = fork();
	if (pid == -1) {
		err(1, "failed fork()");
	}

	if (!pid) {
		setsid();
		_exit(execvp(cmd[0], cmd));
	}

	if (waitpid(pid, &rc, 0) != pid)
		return -1;

	return rc;
}

void cb(int signo)
{
	warnx("got signal %d, calling nft flush ruleset and exit", signo);
}

int main(int argc, char *argv[])
{
	char *load[]  = { "nft", "-f", NULL, NULL };
	char *flush[] = { "nft", "flush", "ruleset", NULL };
	int rc;

	if (argc < 2 || access(argv[1], F_OK))
		errx(1, "Missing nft.conf argument.\nUsage:\n\t%s /path/to/nftables.conf", argv[0]);

	signal(SIGTERM, cb);
	signal(SIGQUIT, cb);
	signal(SIGINT, cb);
	signal(SIGHUP, cb);

	load[2] = argv[1];
	rc = run(load);
	if (rc == -1) {
		err(1, "Internal error while waiting for ruleset to load");
	} else if (WIFEXITED(rc)) {
		rc = WEXITSTATUS(rc);
		if (rc)
			errx(rc, "Failed to load ruleset, exited with status %d", rc);
	} else if (WIFSIGNALED(rc)) {
		errx(rc, "Failed to load ruleset, terminated on signal %d", WTERMSIG(rc));
	}

	warnx("Ruleset active");
	pause();
	run(flush);

	return 0;
}
