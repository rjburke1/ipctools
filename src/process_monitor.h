#ifndef __IPCTOOLS_PROCESS_MONITOR_H__
#define __IPCTOOLS_PROCESS_MONITOR_H__

#include "event_handler.h"
#include "allocator_shm.h"

/**
 * typdef for entry in process monitor table
 */
typedef struct ipt_process_monitor_t ipt_process_monitor_t;
/**
 * typdef for entry in process monitor table 
 */
typedef struct process_monitor_entry_t process_monitor_entry_t;

/**
 * @struct process_monitor_entry_t
 *
 * @brief Entry in the process monitor's shared memory segment.
 */
struct process_monitor_entry_t
{
	/** 
          * The name of the process being montored.
          */
	char name[32];

	/**
          * The fully qualified path to launch the process
          */
	char path[256];

	/**
 	 * The arguments used to launch the process.
         */
	char argv[16][32];

	/**
         * The process id
         */
	unsigned int pid;

	/**
         * The last time the monitored process has updated the shared memory segment to indicate it is alive.
         */
	unsigned int update_time;

	/**
         * The time during which the update_time must be updated.
         */
	unsigned int expire_interval;

	/**
         * This entry is in use. A fixed size buffer of process_entry structures is used.
         */
	int in_use;
};
/**
  * @struct ipt_process_monitor_t
  *
  * @brief  The Process Monitor class is an abstract class used to interface with the process management subsystem.
  *
  * Process Monitor  : A single process that monitors a shared memory segement for a list of active processes.
  * Process Monitored: A process that has registered with the process monitor, and is monitored.
  *
  * The  process manager monitors the last updated timestamps for each registered
  * process. If the last updated time of a process is exceeded, the process is killed and relaunched.
  *
  *                                                      ------------------------
  *							 | shared memory segment |
  *                                                      |     -------------     |
  *                                                      |     | process 1 | <---|------ still_alive --- process monitored 1
  *           process Monitor  -- poll shm segment -->   |     -------------     |  
  *                                                      |     | process 2 | <---|------ still_alive --- process monitored 2
  *                                                      |     -------------     |
  *                 				         -------------------------					
  *         Business Rules
  *         1.   If the time since the last "still_alive" call for a proces exceeds the expiry, the process manager takes action.
  *         2.   The process monitored updates the activity timer by calling still_alive an interval <<  expiry interval
  *	     
  * Each monitored process registers with the process manager and is responsible for calling the keep alive
  */
struct ipt_process_monitor_t
{
	/**
         * Event handler used to monitor process activity.
         */
	ipt_event_handler_t eh;

	/**
         * Called by a process being monitored. It needs to be 
         * called at an interval << the expiry interval
         *
         * @param this process monitor object. 
         * @param name the name of the process to be updated. 
         */
	int (*still_alive)(ipt_process_monitor_t *this, const char *name);

	/**
         * Called to set the expiry interval for a managed process.  
         * called at an interval << the expiry interval
         *
         * @param this process monitor object. 
         * @param name the name of the process to be updated. 
         * @param expire_interval the total elapsed time a process can be inactive before it is considered dead. 
         */
	int (*set_expire_interval)(ipt_process_monitor_t *this, const char *name, unsigned int expire_interval);

        /**
         * Called to register a process with the process monitor subsystem.  
         *
         * @param this process monitor object.
         * @param name the name of the process to be updated.                            
         * @param path the path to the binary.                            
         * @param argv the arguments passed to process when it was launched.                            
         */
         int (*register_process)(ipt_process_monitor_t *this, const char *name, const char *path, const char *const argv[]);

	/**
         * Called to remove a process from the process monitor subsystem.  
         *
         * @param this process monitor object.
         * @param name the name of the process to be updated.         
         */
	int (*remove_process)(ipt_process_monitor_t *this, const char *process_name);

        /**
         * Called to dump the statistics of the process management subystem.
         *
         * @param this process monitor object.
         * @param name the name of the process to be updated.
         */
	void (*dump_stats)(ipt_process_monitor_t *this);

};

/**
 *  Process Monitor constructor
 *
 * @param name the name of the process manager.
 * @param alloc_ptr  a pointer to the allocator.
 */
ipt_process_monitor_t *ipt_process_monitor_create(const char *name, ipt_allocator_t *alloc_ptr);

/**
 *  Process Monitor constructor. This attaches to an existing process management subystem. The
 *  name must already be registered with the allocator from a previous create operation.
 *
 * @param name the name of the process manager.
 * @param alloc_ptr  a pointer to the allocator.
 */
ipt_process_monitor_t *ipt_process_monitor_attach(const char *name, ipt_allocator_t *alloc_ptr);

/**
 *  Process Monitor constructor. This attaches to an existing process management subystem. The
 *  name must already be registered with the allocator from a previous create operation.
 *
 * @param this process monitor object.
 * @param func a function pointer that will be called back once for each element.
 * @param data a block of data that will be passed through to the callback.
 */
void ipt_process_monitor_for_each(ipt_process_monitor_t *this, void (*func)(const process_monitor_entry_t *const entry, const void *data), const void *data);

#endif
