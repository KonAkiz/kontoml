/* SPDX-License-Identifier: MIT */
/*
 * konwinlib.h
 *
 * Copyright (c) 2026 KonAki
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef KONWINLIB_H
#define KONWINLIB_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/*** definitions ***/

typedef struct kon_context kon_context_t;

bool kon_init(void);
void kon_deinit(void);

typedef struct kon_window kon_window_t;

typedef enum kon_windowFlags {
	KON_WINDOW_NONE = 0,
	KON_WINDOW_TRANSPARENT = 1 << 0,
	KON_WINDOW_NO_DECOR = 1 << 1,
	KON_WINDOW_ALWAYS_ONTOP = 1 << 2,
	KON_WINDOW_CENTER = 1 << 3,
	KON_WINDOW_RESIZABLE = 1 << 4
} kon_windowFlags_t;

typedef enum kon_eventType {
	KON_EVENT_NONE = 0,
	KON_EVENT_CLOSE,
	KON_EVENT_RESIZE,
	KON_EVENT_KEY_DOWN,
	KON_EVENT_KEY_UP
} kon_eventType_t;

typedef struct kon_event {
	kon_eventType_t type;
	int width, height; /* will be used to store data for resize events */
	int key;
} kon_event_t;

kon_window_t *kon_createWindow(const char *title, int x, int y, int width, int height, kon_windowFlags_t flags);
void kon_destroyWindow(kon_window_t *window);

bool kon_windowShouldClose(kon_window_t *window);

void kon_setExitKey(kon_window_t *window, int key);

void kon_setWindowPos(kon_window_t *window, int x, int y);
void kon_setWindowSize(kon_window_t *window, int width, int height);

void kon_getWindowPos(kon_window_t *window, int *x, int *y);
void kon_getWindowSize(kon_window_t *window, int *width, int *height);

int kon_pollEvent(kon_window_t *window, kon_event_t *event);
void kon_blitPixels(kon_window_t *window, const uint32_t *pixels, int width, int height);

/*** implementation ***/

#ifdef KONWINLIB_IMPLEMENTATION

static kon_context_t *kon_ctx;

#if defined(__linux__) || defined(__unix__)

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include <string.h>

struct kon_context {
	Display *display;
};

bool kon_init(void) {
	kon_context_t *ctx = malloc(sizeof(kon_context_t));
	if (!ctx) return false;
	ctx->display = XOpenDisplay(NULL);
	if (!ctx->display) return false;
	kon_ctx = ctx;
	return true;
}

void kon_deinit(void) {
	if (!kon_ctx) return;

	XCloseDisplay(kon_ctx->display);
	free(kon_ctx);
	kon_ctx=NULL;
}

struct kon_window {
	Window window;
	GC gc;
	Visual *visual;
	int depth;
	Colormap colormap;
	bool owns_colormap;
	Atom wm_delete;
	bool shouldClose;
	int exitKey;
};

kon_window_t *kon_createWindow(const char *title, int x, int y, int width, int height, kon_windowFlags_t flags) {
	if (!kon_ctx) {
		fprintf(stderr, "kon_createWindow: There appears to be no kon_ctx. \nPlease, do kon_init(); before doing anything with konwinlib, thank you.\n");
		return NULL;
	}

	kon_window_t *window = malloc(sizeof(kon_window_t));
	if (!window) return NULL;

	int screen = DefaultScreen(kon_ctx->display);

	if (flags & KON_WINDOW_CENTER) {
		Window root = RootWindow(kon_ctx->display, screen);

		int mon_x = 0, mon_y = 0;
		int mon_width  = DisplayWidth(kon_ctx->display, screen);
		int mon_height = DisplayHeight(kon_ctx->display, screen);

		Window root_ret, child_ret;
		int root_x, root_y, win_x, win_y;
		unsigned int mask;
		XQueryPointer(kon_ctx->display, root, &root_ret, &child_ret, &root_x, &root_y, &win_x, &win_y, &mask);

		XRRScreenResources *res = XRRGetScreenResourcesCurrent(kon_ctx->display, root);
		if (res) {
			for (int i = 0; i < res->noutput; i++) {
				XRROutputInfo *out_info = XRRGetOutputInfo(kon_ctx->display, res, res->outputs[i]);

				if (out_info && out_info->crtc) {
					XRRCrtcInfo *crtc = XRRGetCrtcInfo(kon_ctx->display, res, out_info->crtc);
					if (crtc) {

						if (root_x >= crtc->x && root_x < crtc->x + (int)crtc->width && 
							root_y >= crtc->y && root_y < crtc->y + (int)crtc->height) {
							mon_x = crtc->x;
							mon_y = crtc->y;
							mon_width = crtc->width;
							mon_height = crtc->height;
							XRRFreeCrtcInfo(crtc);
							if (out_info) XRRFreeOutputInfo(out_info);
							break;
						}
						XRRFreeCrtcInfo(crtc);
					}
				}
				if (out_info) XRRFreeOutputInfo(out_info);
			}
			XRRFreeScreenResources(res);
		}

		x = mon_x + (mon_width  - width ) / 2;
		y = mon_y + (mon_height - height) / 2;
	}

	if (flags & KON_WINDOW_TRANSPARENT) {
		XVisualInfo vinfo;
		if (!XMatchVisualInfo(kon_ctx->display, screen, 32, TrueColor, &vinfo)) {
			free(window);
			return NULL;
		}

		window->visual   = vinfo.visual;
		window->depth    = vinfo.depth;
		window->colormap = XCreateColormap(kon_ctx->display, RootWindow(kon_ctx->display, screen), window->visual, AllocNone);

		XSetWindowAttributes attrs;
		attrs.colormap = window->colormap;
		attrs.background_pixel = 0;
		attrs.border_pixel = 0;

		window->window = XCreateWindow(
									   kon_ctx->display, RootWindow(kon_ctx->display, screen),
									   x, y, width, height, 0, window->depth, InputOutput,
									   window->visual, CWColormap | CWBackPixel | CWBorderPixel, &attrs);

		window->owns_colormap = true;
	} else {
		window->visual   = DefaultVisual(kon_ctx->display, screen);
		window->depth    = DefaultDepth(kon_ctx->display, screen);
		window->colormap = DefaultColormap(kon_ctx->display, screen);

		window->window = XCreateSimpleWindow(
											 kon_ctx->display, RootWindow(kon_ctx->display, screen),
											 x, y, width, height, 0, 0, 0);

		window->owns_colormap = false;
	}

	if (!window->window) {
		if (window->owns_colormap) {
			XFreeColormap(kon_ctx->display, window->colormap);
		}
		free(window);
		return NULL;
	}

	XStoreName(kon_ctx->display, window->window, title);
	XSelectInput(kon_ctx->display, window->window, ExposureMask | StructureNotifyMask | KeyPressMask | KeyReleaseMask );

	if (flags & KON_WINDOW_NO_DECOR) {
		Atom motif_hints = XInternAtom(kon_ctx->display, "_MOTIF_WM_HINTS", False);
		struct { unsigned long flags, functions, decorations; long input_mode; unsigned long status; } hints;
		memset(&hints, 0, sizeof(hints));
		hints.flags = 2;
		hints.decorations = 0;
		XChangeProperty(kon_ctx->display, window->window, motif_hints,
						motif_hints, 32, PropModeReplace, (unsigned char *)&hints, 5);
	}

	if (flags & KON_WINDOW_ALWAYS_ONTOP) {
		Atom wm_state = XInternAtom(kon_ctx->display, "_NET_WM_STATE", False);
		Atom wm_above = XInternAtom(kon_ctx->display, "_NET_WM_STATE_ABOVE", False);
		XChangeProperty(kon_ctx->display, window->window, wm_state, XA_ATOM, 32,
						PropModeReplace, (unsigned char *)&wm_above, 1);
	}

	if (!(flags & KON_WINDOW_RESIZABLE)) {
		XSizeHints hints;
		hints.flags = PMinSize | PMaxSize;
		hints.min_width  = hints.max_width  = width;
		hints.min_height = hints.max_height = height;
		XSetWMNormalHints(kon_ctx->display, window->window, &hints);
	}

	window->wm_delete = XInternAtom(kon_ctx->display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(kon_ctx->display, window->window, &window->wm_delete, 1);

	window->gc = XCreateGC(kon_ctx->display, window->window, 0, NULL);

	XMapWindow(kon_ctx->display, window->window);
	XFlush(kon_ctx->display);

	window->exitKey = 0;

	window->shouldClose = false;

	return window;
}

void kon_destroyWindow(kon_window_t *window) {
	if (!window) return;
	if (!kon_ctx) { free(window); return; }

	if (window->gc) XFreeGC(kon_ctx->display, window->gc);
	if (window->window) XDestroyWindow(kon_ctx->display, window->window);
	if (window->owns_colormap && window->colormap) XFreeColormap(kon_ctx->display, window->colormap);

	XFlush(kon_ctx->display);

	free(window);
}

void kon_setExitKey(kon_window_t *window, int key) {
	if (!window) return;
	window->exitKey = key;
}

void kon_setWindowPos(kon_window_t *window, int x, int y) {
	if (!window || !kon_ctx) return;

	XMoveWindow(kon_ctx->display, window->window, x, y);
	XFlush(kon_ctx->display);
}

void kon_setWindowSize(kon_window_t *window, int width, int height) {
	if (!window || !kon_ctx) return;

	XResizeWindow(kon_ctx->display, window->window, width, height);
	XFlush(kon_ctx->display);
}

void kon_getWindowPos(kon_window_t *window, int *x, int *y) {
	if (!window || !kon_ctx || !x || !y) return;

	Window child;
	XTranslateCoordinates(kon_ctx->display, window->window, RootWindow(kon_ctx->display, DefaultScreen(kon_ctx->display)),
						  0, 0, x, y, &child);
}

void kon_getWindowSize(kon_window_t *window, int *width, int *height) {
	if (!window || !kon_ctx || !width || !height) return;

	XWindowAttributes attrs;
	XGetWindowAttributes(kon_ctx->display, window->window, &attrs);
	*width  = attrs.width;
	*height = attrs.height;
}

int kon_pollEvent(kon_window_t *window, kon_event_t *event) {
	if (!window || !kon_ctx || !event) return 0;

	event->type = KON_EVENT_NONE;

	if (!XPending(kon_ctx->display)) return 0;

	XEvent xev;
	XNextEvent(kon_ctx->display, &xev);

	if (xev.type == ClientMessage) {
		if ((Atom)xev.xclient.data.l[0] == window->wm_delete) {
			event->type = KON_EVENT_CLOSE;
			window->shouldClose = true;
		}
	}

	if (xev.type == ConfigureNotify) {
		event->type = KON_EVENT_RESIZE;
		event->width = xev.xconfigure.width;
		event->height = xev.xconfigure.height;
		return 1;
	}

	if (xev.type == KeyPress) {
		event->type = KON_EVENT_KEY_DOWN;
		event->key = (int)XLookupKeysym(&xev.xkey, 0);
		if (window->exitKey != 0 && event->key == window->exitKey) {
			window->shouldClose = true;
		}
		return 1;
	}

	if (xev.type == KeyRelease) {
		event->type = KON_EVENT_KEY_UP;
		event->key = (int)XLookupKeysym(&xev.xkey, 0);
		return 1;
	}

	return 1;
}

bool kon_windowShouldClose(kon_window_t *window) {
	if (!window || !kon_ctx) return true;
	return window->shouldClose;
}

/* TODO: implement the rest of these */

void kon_blitPixels(kon_window_t *window, const uint32_t *pixels, int width, int height) {
	if (!window || !kon_ctx || !pixels) return;

	XImage *ximg = XCreateImage(kon_ctx->display, window->visual, window->depth, ZPixmap, 0, (char *)pixels, width, height, 32, 0);
	if (!ximg) return;

	XPutImage(kon_ctx->display, window->window, window->gc, ximg, 0, 0, 0, 0, width, height);

	ximg->data = NULL;
	XDestroyImage(ximg);

	XFlush(kon_ctx->display);
}

/*** windows implementation ***/

#elif defined(_WIN32)

#include <windows.h>

struct kon_context {
	HINSTANCE hInstance;
};

struct kon_window {
	HWND hwnd;
	bool shouldClose;
	bool hasResizeEvent;
	int resizeWidth, resizeHeight;
	bool isTransparent;
	bool hasKeyEvent;
	kon_eventType_t keyEventType;
	int keyEventKey;
	int exitKey;
};

LRESULT CALLBACK kon_wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	kon_window_t *window = (kon_window_t *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (msg) {
	case WM_NCCREATE: {
		CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
		break;
	}
	case WM_KEYDOWN:
		if (window) {
			window->hasKeyEvent = true;
			window->keyEventType = KON_EVENT_KEY_DOWN;
			window->keyEventKey = (int)wParam;
			if (window->exitKey != 0 && (int)wParam == window->exitKey) {
				window->shouldClose = true;
			}
		}
		break;
	case WM_KEYUP:
		if (window) {
			window->hasKeyEvent = true;
			window->keyEventType = KON_EVENT_KEY_UP;
			window->keyEventKey = (int)wParam;
		}
		break;
	case WM_CLOSE:
		if (window) window->shouldClose = true;
		break;
	case WM_SIZE:
		if (window) {
			window->hasResizeEvent = true;
			window->resizeWidth  = LOWORD(lParam);
			window->resizeHeight = HIWORD(lParam);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool kon_init(void) {
	kon_context_t *ctx = malloc(sizeof(kon_context_t));
	if (!ctx) return false;

	ctx->hInstance = GetModuleHandle(NULL);

	WNDCLASSEX wc = {0};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = kon_wndProc;
	wc.hInstance = ctx->hInstance;
	wc.lpszClassName = "KonWinLibClass";
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	if (!RegisterClassEx(&wc)) {
		free(ctx);
		return false;
	}

	kon_ctx = ctx;
	return true;
}

void kon_deinit(void) {
	if (!kon_ctx) return;

	free(kon_ctx);
	kon_ctx = NULL;
}


kon_window_t *kon_createWindow(const char *title, int x, int y, int width, int height, kon_windowFlags_t flags) {
	if (!kon_ctx) return NULL;

	kon_window_t *window = malloc(sizeof(kon_window_t));
	if (!window) return NULL;

	window->shouldClose = false;
	window->hasResizeEvent = false;
	window->isTransparent = (flags & KON_WINDOW_TRANSPARENT);
	window->exitKey = 0;

	DWORD style;
	if (flags & KON_WINDOW_NO_DECOR) {
		style = WS_POPUP;
	} else {
		style = WS_OVERLAPPEDWINDOW;
		if (!(flags & KON_WINDOW_RESIZABLE)) {
			style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
		}
	}

	DWORD exStyle = 0;
	if (flags & KON_WINDOW_ALWAYS_ONTOP) exStyle |= WS_EX_TOPMOST;
	if (flags & KON_WINDOW_TRANSPARENT)  exStyle |= WS_EX_LAYERED;

	if (flags & KON_WINDOW_CENTER) {
		POINT cursor;
		GetCursorPos(&cursor);

		HMONITOR hmon = MonitorFromPoint(cursor, MONITOR_DEFAULTTONEAREST);

		MONITORINFO mi;
		mi.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(hmon, &mi);

		int mon_x = mi.rcMonitor.left;
		int mon_y = mi.rcMonitor.top;
		int mon_width = mi.rcMonitor.right - mi.rcMonitor.left;
		int mon_height = mi.rcMonitor.bottom - mi.rcMonitor.top;

		x = mon_x + (mon_width  - width ) / 2;
		y = mon_y + (mon_height - height) / 2;
	}

	window->hwnd = CreateWindowEx(exStyle, "KonWinLibClass", title, style, x, y, width, height, NULL, NULL, kon_ctx->hInstance, window);

	if (!window->hwnd) {
		free(window);
		return NULL;
	}

	ShowWindow(window->hwnd, SW_SHOW);

	return window;
}

void kon_destroyWindow(kon_window_t *window) {
	if (!window) return;
	if (window->hwnd) DestroyWindow(window->hwnd);
	free(window);
}

bool kon_windowShouldClose(kon_window_t *window) {
	if (!window) return true;
	return window->shouldClose;
}


void kon_setExitKey(kon_window_t *window, int key) {
	if (!window) return;
	window->exitKey = key;
}

void kon_setWindowPos(kon_window_t *window, int x, int y) {
	if (!window || !kon_ctx) return;

	SetWindowPos(window->hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void kon_setWindowSize(kon_window_t *window, int width, int height) {
	if (!window || !kon_ctx) return;

	SetWindowPos(window->hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
}


void kon_getWindowPos(kon_window_t *window, int *x, int *y) {
	if (!window || !x || !y) return;

	RECT rect;
	GetWindowRect(window->hwnd, &rect);
	*x = rect.left;
	*y = rect.top;
}

void kon_getWindowSize(kon_window_t *window, int *width, int *height) {
	if (!window || !width || !height) return;

	RECT rect;
	GetWindowRect(window->hwnd, &rect);
	*width  = rect.right  - rect.left;
	*height = rect.bottom - rect.top;
}

int kon_pollEvent(kon_window_t *window, kon_event_t *event) {
	if (!window || !event) return 0;

	event->type = KON_EVENT_NONE;

	MSG msg;
	if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) return 0;

	TranslateMessage(&msg);
	DispatchMessage(&msg);

	if (window->hasResizeEvent) {
		event->type = KON_EVENT_RESIZE;
		event->width  = window->resizeWidth;
		event->height = window->resizeHeight;
		window->hasResizeEvent = false;
		return 1;
	}

	if (window->shouldClose) {
		event->type = KON_EVENT_CLOSE;
		return 1;
	}

	if (window->hasKeyEvent) {
		event->type = window->keyEventType;
		event->key  = window->keyEventKey;
		window->hasKeyEvent = false;
		return 1;
	}

	return 1;
}

void kon_blitPixels(kon_window_t *window, const uint32_t *pixels, int width, int height) {
	if (!window || !pixels) return;

	HDC screenDC = GetDC(NULL);
	HDC memDC = CreateCompatibleDC(screenDC);

	BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height; /* did this because... windows for some reason draws from the bottom up... -_- */
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	void *dibPixels;
	HBITMAP hbm = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &dibPixels, NULL, 0);
	memcpy(dibPixels, pixels, (size_t)width * height * 4);

	HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, hbm);

	if (window->isTransparent) {
		POINT srcPos = {0, 0};
		SIZE size = {width, height};
		POINT dstPos;
		RECT winRect;
		GetWindowRect(window->hwnd, &winRect);
		dstPos.x = winRect.left;
		dstPos.y = winRect.top;

		BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
		UpdateLayeredWindow(window->hwnd, screenDC, &dstPos, &size, memDC, &srcPos, 0, &blend, ULW_ALPHA);
	} else {
		HDC winDC = GetDC(window->hwnd);
		BitBlt(winDC, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
		ReleaseDC(window->hwnd, winDC);
	}

	SelectObject(memDC, oldBmp);
	DeleteObject(hbm);
	DeleteDC(memDC);
	ReleaseDC(NULL, screenDC);
}

#else
	#error "konwinlib.h: unsupported platform"
#endif /* end of platform switch */

#endif /* end of KONWINLIB_IMPLEMENTATION */

#endif
