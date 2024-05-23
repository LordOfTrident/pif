#include "demo.c"

class DemoBase : protected DemoData {
protected:
	DemoBase(const char *title, int scrW, int scrH, float scale,
	         float shading, const char *palPath) {
		demoSetup((DemoData*)this, title, scrW, scrH, scale, shading, palPath);
	}

	virtual ~DemoBase() {
		demoCleanup((DemoData*)this);
	}

	virtual void render() {}
	virtual void update() {}
	virtual void handleEvent(SDL_Event *evt) {(void)evt;}

public:
	void run() {
		while (running) {
			demoFrame((DemoData*)this);

			render();
			demoDisplay((DemoData*)this);

			SDL_Event evt;
			while (SDL_PollEvent(&evt)) {
				demoEvent((DemoData*)this, &evt);
				handleEvent(&evt);
			}

			update();
		}
	}
};
