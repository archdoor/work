#include <signal.h> 
#include <unistd.h> 
#include "system.h"
#include "log.h"

static int g_s32CanRun = 1;
static int g_s32Reconfig = 0;

void MySignalHandler(int s32Sig)
{
	if( s32Sig == SIGTERM || s32Sig == SIGINT ) 
    {
		g_s32CanRun = 0;
	}
    else if( s32Sig == SIGHUP )
    {
		g_s32Reconfig = 1;
	}
}

#ifdef DEBUG

int main(int argc, char **argv)
{
	sigset_t set;

	signal(SIGTERM,MySignalHandler);
	signal(SIGINT,MySignalHandler);
	signal(SIGHUP,MySignalHandler);
	signal(SIGUSR1,MySignalHandler);
	signal(SIGUSR2,MySignalHandler);
	signal(SIGPIPE,SIG_IGN);

	register int c;
    while( (c = getopt(argc, argv, "v")) != -1 )
    {
        switch(c)
        {
            case 'v':
                LogDebug("%s version: %s\n", PROC_NAME, VERSION);
                exit(0);
                break;
            default:
                break;
        }
    }

	if( SystemInit(argc, argv) != 0 )
    {
		SystemDeinit();
		return -1;
	}
    else
    {
		sigemptyset(&set);
		while( g_s32CanRun == 1 )
        {
			sigsuspend(&set);
			if( g_s32Reconfig == 1 )
            {
				g_s32Reconfig = 0;
			}
		}
		SystemDeinit();
	}

	return 0;
}

#endif

