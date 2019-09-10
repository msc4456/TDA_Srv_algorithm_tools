#ifndef __SRV_VIEWS_H
#define __SRV_VIEWS_H

#define MAX_SRV_VIEWS 10

#define SRV_VIEW_ZOOMED_OUT 0
#define SRV_VIEW_ZOOMED_IN 1
#define SRV_VIEW_FRONT 2
#define SRV_VIEW_BACK 3
#define SRV_VIEW_LEFT 4
#define SRV_VIEW_RIGHT 5
#define SRV_VIEW_BS_LEFT 6
#define SRV_VIEW_BS_RIGHT 7

typedef struct _srv_coords_t
{
	float camx;
	float camy;
	float camz;
	float targetx;
	float targety;
	float targetz;
	float anglex;
	float angley;
	float anglez;
} srv_coords_t;

void srv_views_init();
#endif
