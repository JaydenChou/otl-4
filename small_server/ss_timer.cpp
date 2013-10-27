#include "ss_timer.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

int ss_timer_init(pss_timer_t timer)
{
	if (NULL == timer) {
		return -1;
	}

	timer->timer_cur = 0;
	return 0;
}

int ss_timer_reset(pss_timer_t timer)
{
	if (NULL == timer) {
		return -1;
	}

	timer->timer_cur = 0;
	return 0;
}

int ss_timer_settask(pss_timer_t timer, char *task, int endprev)
{
	if (NULL == timer || NULL == task || task[0] == '\0') {
		return -1;
	}

	if (timer->timer_cur >= MAX_TIMER_COUNT-1) {
		return -1;
	}

	//if user's set to end previous, end it
	if (endprev) {
		ss_timer_endtask(timer);
	}

	//if task is exist
	unsigned int i;
	for (i = 0; i < timer->timer_cur; ++i) {
		if (!strncmp(timer->timername[i], task, sizeof(timer->timername[0]))) {
			break;
		}
	}

	struct timeval cur_time;

	gettimeofday(&cur_time, NULL);
	memcpy(&timer->timervalue[i], &cur_time, sizeof(timeval));
	timer->mstimer[i] = 0;
	timer->timerstatus[i] = SS_TIMER_START;
	strncpy(timer->timername[i], task, sizeof(timer->timername[0]));
	timer->timername[i][sizeof(timer->timername[0])-1] = '\0';

	//new timer?
	if (i == timer->timer_cur) {
		++timer->timer_cur;
	}

	return 0;
}

int ss_timer_endtask(pss_timer_t timer)
{
	if (NULL == timer) {
		return -1;
	}

	if (timer->timer_cur >= MAX_TIMER_COUNT) {
		return -1;
	}

	struct timeval cur_time;
	gettimeofday(&cur_time, NULL);
	unsigned int i;
	for (i = 0; i < timer->timer_cur; ++i) {
		if (timer->timerstatus[i] == SS_TIMER_START) {
			timer->timerstatus[i] = SS_TIMER_END;
			timer->mstimer[i] = (cur_time.tv_sec - timer->timervalue[i].tv_sec)*1000 + (cur_time.tv_usec - timer->timervalue[i].tv_usec)/1000;
		}
	}

	return 0;
}

int ss_timer_gettask(pss_timer_t timer, char *task)
{
	if (NULL == timer || NULL == task) {
		return -1;
	}

	if (NULL == task || task[0] == '\0') {
		return -1;
	}

	if (timer->timer_cur >= MAX_TIMER_COUNT-1) {
		return -1;
	}

	ss_timer_endtask(timer);

	unsigned int i;
	for (i = 0; i < timer->timer_cur; ++i) {
		if (!strncmp(timer->timername[i], task, sizeof(timer->timername[0]))) {
			break;
		}
	}

	if (i == timer->timer_cur) {
		if (0 != strncasecmp(task, TOTAL_TIMER_RES, sizeof(TOTAL_TIMER_RES))) {
			return -1;
		}
		unsigned total = 0;

		for (i = 0; i < timer->timer_cur; ++i) {
			total += timer->mstimer[i];
		}

		return total;
	}

	return timer->mstimer[i];

}

int ss_timer_gettaskstring(pss_timer_t timer, char *task, char *dest, size_t len)
{
	if (NULL == timer || NULL == task || task[0] == '\0') {
		return -1;
	}

	unsigned int ms = ss_timer_gettask(timer, task);
	snprintf(dest, len, "%s:%ums", task, ms);
	dest[len-1] = '\0';

	return 0;
}

char *ss_timer_gettaskformat(pss_timer_t timer)
{
	if (NULL == timer) {
		return NULL;
	}

	if (timer->timer_cur >= MAX_TIMER_COUNT) {
		return NULL;
	}
	ss_timer_endtask(timer);
	timer->formatedstr[0] = '\0';

	char temp[40];
	unsigned int i;
	unsigned int total = 0;
	for (i = 0; i < timer->timer_cur; ++i) {
		total += timer->mstimer[i];
		snprintf(temp, sizeof(temp), "%s:%ums ", timer->timername[i], timer->mstimer[i]);
		strncat(timer->formatedstr, temp, strlen(temp));
	}

	snprintf(temp, sizeof(temp), TOTAL_TIMER_RES":%ums", total);
	strncat(timer->formatedstr, temp, strlen(temp));

	return timer->formatedstr;
}
