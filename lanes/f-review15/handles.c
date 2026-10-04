#include <adelefeld.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    ulong words[] = {0, 1, 4, 9, 18446744073709551615UL};
    unsigned failures = 0, checks = 0;
    adf_place_t mark;
    if (adf_place_prime(&mark, 97)) return 2;
    for (unsigned j = 0; j < sizeof(words) / sizeof(*words); j++) {
        adf_place_t p = mark;
        checks++;
        if (adf_place_prime(&p, words[j]) != ADF_DOMAIN || !adf_place_equal(p, mark)) failures++;
    }
    for (unsigned j = 1; j < sizeof(words) / sizeof(*words); j++) {
        for (int limit = 0; limit < 2; limit++) {
            pid_t pid = fork();
            if (pid == 0) {
                acb_t s, y;
                acb_init(s); acb_init(y); acb_one(s);
                adf_place_t v = {words[j]};
                int st = adf_local_zeta_factor_at(y, NULL, s, v, limit ? 2097153 : 64);
                _exit(limit && st == ADF_LIMIT ? 43 : 42);
            }
            int st;
            if (pid < 0 || waitpid(pid, &st, 0) != pid) return 2;
            checks++;
            if (limit ? !(WIFEXITED(st) && WEXITSTATUS(st) == 43) :
                        !(WIFSIGNALED(st) && WTERMSIG(st) == SIGABRT)) failures++;
        }
    }
    printf("handle checks=%u failures=%u\n", checks, failures);
    flint_cleanup(); return failures != 0;
}
