#ifndef _SS_TIMER_H_
#define _SS_TIMER_H_

#include <sys/time.h>
#include <cstddef>
#define MAX_TIMER_COUNT 64 //max timer number in one thread
#define MAX_TIMER_NAMELEN 32 //the length of timer's name 

#define TOTAL_TIMER_RES "ss_total"


enum ss_timer_status {
	SS_TIMER_START,	//start count
	SS_TIMER_END	//end count
};

//timer info
typedef struct _ss_timer_t {
	unsigned int timer_cur;			//current timer count
	struct timeval timervalue[MAX_TIMER_COUNT];	//timer info matrix
	int mstimer[MAX_TIMER_COUNT];	//timer info matrix
	ss_timer_status timerstatus[MAX_TIMER_COUNT];	//timer info matrix
	char timername[MAX_TIMER_COUNT][MAX_TIMER_NAMELEN];	//timer name
	char formatedstr[MAX_TIMER_COUNT*20];		//formated string out
} ss_timer_t, *pss_timer_t;

/**
 * @brief init function
 *	 run before thread start
 * @param [in/out] the pointer of timer
 * @return 0 success -1 fail
 */
int ss_timer_init(pss_timer_t timer);

/**
 * @brief it will be call program end, use to close log
 * @param[in] the pointer of timer
 * @return 0 success -1 failed
 */
int ss_timer_reset(pss_timer_t timer);

/**
 * @brief start timer. to count;
 * @param[in] timer: the pointer of timer
 * @param[in] task : the name of task
 * @param[in] endprev : end previous timer or not
 * @return 0 success -1 failed
 */
int ss_timer_settask(pss_timer_t timer, char *task, int endprev = 1);

/**
 * @brief to end timer
 * @param[in] timer : the pointer of timer
 * @return  number of char been added
 */
int ss_timer_endtask(pss_timer_t timer);

/**
 * @brief get timer info
 * @param[in] timer : the pointer of timer
 * @param[in] task : the name of task
 * @param[in] the timer in ms
 */
int ss_timer_gettask(pss_timer_t timer, char *task);

/**
 * @brief get timer info
 * @param[in] task : the name of task
 * @param[in] timer : the pointer of timer
 * @param[out] dest : the info return
 * @param[in] dest_len : the length of return
 * @return 0 success -1 failed
 */
int ss_timer_gettaskstring(pss_timer_t timer, char *task, char *dest, size_t len);

/**
 * @brief get timer info format string
 * @param[in] timer : the pointer of timer
 * @return timer in name. eg name:5ms  name:6ms
 */
char *ss_timer_gettaskformat(pss_timer_t timer);
#endif

