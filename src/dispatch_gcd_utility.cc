/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#if !(defined(LIBBIO_NO_DISPATCH) && LIBBIO_NO_DISPATCH)

#include <libbio/dispatch/utility.hh>
#include <sys/wait.h>


namespace libbio {
	
	void install_dispatch_sigchld_handler(dispatch_queue_t queue, dispatch_block_t block)
	{
		static dispatch_once_t once_token;
		static dispatch_source_t signal_source;
		
		dispatch_once(&once_token, ^{
			signal_source = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, SIGCHLD, 0, queue);
			dispatch_source_set_event_handler(signal_source, block);
			dispatch_resume(signal_source);
		});
	}
	
	
	void install_dispatch_sigchld_handler(dispatch_queue_t queue, sigchld_handler &handler)
	{
		install_dispatch_sigchld_handler(queue, ^{
			bool did_report_error(false);
			while (true)
			{
				int status(0);
				auto const pid(::waitpid(-1, &status, WNOHANG | WUNTRACED));
				if (pid <= 0)
					break;
		
				if (WIFEXITED(status))
				{
					auto const exit_status(WEXITSTATUS(status));
					if (0 != exit_status)
					{
						did_report_error = true;
						char const *reason{nullptr};
						switch (exit_status)
						{
							case 127:
								reason = "command not found";
								break;
					
							case 126:
								reason = "command invoked cannot execute";
								break;
					
							case 69: // EX_UNAVAILABLE
								reason = "service unavailable";
								break;
					
							case 71: // EX_OSERR
								reason = "unknown error from execvp()";
								break;
					
							case 74: // EX_IOERR
								reason = "an I/O error occurred";
								break;
					
							default:
								break;
						}
						
						handler.child_did_exit_with_nonzero_status(pid, exit_status, reason);
					}
				}
				else if (WIFSIGNALED(status))
				{
					did_report_error = true;
					auto const signal_number(WTERMSIG(status));
					handler.child_received_signal(pid, signal_number);
				}
			}
			
			handler.finish_handling(did_report_error);
		});
	}
}

#endif
