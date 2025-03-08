#include <cctype>
#include <algorithm>
#include <string>
#include <vector>
#include <filesystem>
#include <SDL.h>
#include <SDL_image.h>

using namespace std;

bool is_little_endian(void)
{
	const uint16_t value = 0b1;
	const uint8_t* const ptr = (uint8_t*) &value;
	return ptr[0];
}

void invert(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a)
{
	r = 0xff - r;
	g = 0xff - g;
	b = 0xff - b;
}

void swap_rg(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a)
{
	r = r ^ g;
	g = r ^ g;
	r = r ^ g;
}

void swap_gb(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a)
{
	g = g ^ b;
	b = g ^ b;
	g = g ^ b;
}

void swap_br(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a)
{
	b = b ^ r;
	r = b ^ r;
	b = b ^ r;
}

void cycle(uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a)
{
	r = r ^ g ^ b;
	g = r ^ g ^ b;
	b = r ^ g ^ b;
	r = r ^ g ^ b;
}

void set_title(SDL_Window* const window, const int mode, filesystem::path location)
{
    switch (mode % 3)
    {
        case 0: SDL_SetWindowTitle(window, string(location.stem().string()).c_str()); return;
        case 1: SDL_SetWindowTitle(window, string(location.filename().string()).c_str()); return;
        case 2: SDL_SetWindowTitle(window, string(filesystem::absolute(location).string()).c_str()); return;
    }
}

struct render_computation
{
    SDL_Window* window;
    SDL_Renderer* renderer;
	SDL_Surface* o_image;
    SDL_Texture* image = nullptr;
    int img_w, img_h, win_w, win_h, x_offset = 0, y_offset = 0;
    float zoom = 1, rotation = 0;
    bool original_size = false, flip_h = false, flip_v = false, light_mode = false;

    render_computation(SDL_Window* win, SDL_Renderer* ren, SDL_Surface* img) : window(win), renderer(ren), o_image(img)
	{
		this->refresh_sizes();
		this->redraw_target();
	}


    void refresh_screen(void)
    {
        int w = this->img_w * this->zoom, h = this->img_h * this->zoom;
        if (!this->original_size)
        {
            float scale = SDL_min(float(this->win_w) / this->img_w, float(this->win_h) / this->img_h);      // Scale that fits to width.
            w = this->img_w * scale * this->zoom;
            h = this->img_h * scale * this->zoom;
        }
        SDL_Rect dest = {(this->win_w - w)/2, (this->win_h - h)/2, w, h};
        this->apply_limit(this->r_bound(w, h, this->rotation));      // Limits are applied based on the rotated bounds.

        dest.x -= this->x_offset;
        dest.y -= this->y_offset;

        SDL_RenderClear(this->renderer);
        SDL_RenderCopyEx(this->renderer, this->image, nullptr, &dest, this->rotation, nullptr, SDL_RendererFlip(((this->flip_h) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE) | ((this->flip_v) ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE)));
        SDL_RenderPresent(this->renderer);
    }

    void refresh_sizes(void)
    {
		if (this->o_image)
		{
			this->img_w = this->o_image->w;
			this->img_h = this->o_image->h;
		}
        SDL_GetWindowSize(this->window, &this->win_w, &this->win_h);
    }

    void refresh_size(const char mode)
    {
        switch (mode)
        {
            case 'I':
				if (this->o_image)
				{
					this->img_w = this->o_image->w;
					this->img_h = this->o_image->h;
				}
				break;

            case 'W':
				SDL_GetWindowSize(this->window, &this->win_w, &this->win_h);
				break;
        }
    }

    void scale(float value)
    {
        this->zoom += value;
        if ((value < 0 && this->zoom == 0) || this->zoom < 0) this->zoom = 0.01;
    }

    void scale_reset(void)
    {
        this->zoom = 1;
        this->flip_h = false;
        this->flip_v = false;
        this->x_offset = 0;
        this->y_offset = 0;
        this->rotation = 0;
    }

    void toggle_fullscreen(void)
    {
        SDL_SetWindowFullscreen(this->window, (SDL_GetWindowFlags(this->window) & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_ShowCursor((SDL_ShowCursor(SDL_QUERY)) ? SDL_DISABLE : SDL_ENABLE);
    }

    void apply_limit(SDL_Rect dest)     // Limits are now applied accurately.
    {
        if (dest.w > this->win_w)
        {
            if (this->x_offset < -(dest.w - this->win_w)/2) this->x_offset = -(dest.w - this->win_w)/2;
            else if (this->x_offset > (dest.w - this->win_w)/2) this->x_offset = (dest.w - this->win_w)/2;
        } else x_offset = 0;

        if (dest.h > this->win_h)
        {
            if (this->y_offset < -(dest.h - this->win_h)/2) this->y_offset = -(dest.h - this->win_h)/2;
            else if (this->y_offset > (dest.h - this->win_h)/2) this->y_offset = (dest.h - this->win_h)/2;
        } else y_offset = 0;
    }

    static SDL_Rect r_bound(int w, int h, double angle)
    {
        angle *= M_PI/180;

        SDL_Point cnt = {(w/2), (h/2)};
        SDL_Point corners[4] =
        {
            {-cnt.x, -cnt.y},
            {cnt.x, -cnt.y},
            {-cnt.x, cnt.y},
            {cnt.x, cnt.y}
        };

        int min_x = INT_MAX, max_x = INT_MIN, min_y = INT_MAX, max_y = INT_MIN;
        for (int i = 0; i < 4; ++i)
        {
            int x = corners[i].x;
            int y = corners[i].y;
            int rotated_x = x * SDL_cosf(angle) - y * SDL_sinf(angle) /* <- Rotation | Translation -> */ + cnt.x;
            int rotated_y = x * SDL_sinf(angle) + y * SDL_cosf(angle) /* <- Rotation | Translation -> */ + cnt.y;

            min_x = SDL_min(min_x, rotated_x);
            max_x = SDL_max(max_x, rotated_x);
            min_y = SDL_min(min_y, rotated_y);
            max_y = SDL_max(max_y, rotated_y);
        }

        return {min_x, min_y, max_x - min_x, max_y - min_y};
    }


    static SDL_Rect fit_to_screen(const int img_w, const int img_h, const int scr_w, const int scr_h)
    {
        SDL_Rect dest;
        float scale = SDL_min(float(scr_w) / img_w, float(scr_h) / img_h);

        dest.w = img_w * scale;
        dest.h = img_h * scale;
        dest.x = (scr_w - dest.w) / 2;
        dest.y = (scr_h - dest.h) / 2;

        return dest;
    }

	void apply_effect(char fx)
	{
		switch (fx)
		{
			case 'i':
				apply_effect(invert);
				break;
			case 'c':
				apply_effect(cycle);
				break;
			case 'r':
				apply_effect(swap_gb);
				break;
			case 'g':
				apply_effect(swap_br);
				break;
			case 'b':
				apply_effect(swap_rg);
				break;
		}
		this->redraw_target();
	}

	void apply_effect(void (*effect)(uint8_t&, uint8_t&, uint8_t&, uint8_t&))
	{
		if (SDL_MUSTLOCK(this->o_image) && SDL_LockSurface(this->o_image)) return;

		uint8_t* pixels = (uint8_t*) this->o_image->pixels;		// Pixels array; assuming minimum of 8-bit (1 byte).
		int pitch = this->o_image->pitch, pixel_byte = this->o_image->format->BytesPerPixel;
		for (int y = 0; y < this->o_image->h; ++y)
			for (int x = 0; x < this->o_image->w; ++x)
			{
				uint8_t* pixel_address = pixels + (y * pitch) + (x * pixel_byte);
				uint32_t pixel_value;

				switch (pixel_byte)
				{
					case 1:		// 8-bit
						pixel_value = *pixel_address;
						break;

					case 2:		// 16-bit
						pixel_value = *((uint16_t*) pixel_address);
						break;

					case 3:		// 24-bit
						is_little_endian() ?
							pixel_value = (pixel_address[0]) | (pixel_address[1] << 8) | (pixel_address[2] << 16) :
							pixel_value = (pixel_address[0] << 16) | (pixel_address[1] << 8) | (pixel_address[2]);
						break;

					case 4:		// 32-bit
						pixel_value = *((uint32_t*) pixel_address);
						break;

					default: continue;	// Unsupported type.
				}

				uint8_t r, g, b, a;
				SDL_GetRGBA(pixel_value, this->o_image->format, &r, &g, &b, &a);
				effect(r, g, b, a);
				pixel_value = SDL_MapRGBA(this->o_image->format, r, g, b, a);

				switch (pixel_byte)
				{
					case 1:
						*pixel_address = (uint8_t) pixel_value;
						break;

					case 2:
						*((uint16_t*) pixel_address) = (uint16_t) pixel_value;
						break;

					case 3:
						if (is_little_endian())
						{
							pixel_address[0] = pixel_value & 0xFF;
							pixel_address[1] = (pixel_value >> 8) & 0xFF;
							pixel_address[2] = (pixel_value >> 16) & 0xFF;
						}
						else
						{
							pixel_address[0] = (pixel_value >> 16) & 0xFF;
							pixel_address[1] = (pixel_value >> 8) & 0xFF;
							pixel_address[2] = pixel_value & 0xFF;
						}
						break;

					case 4:
						*((uint32_t*) pixel_address) = pixel_value;
						break;
				}
			}

		if (SDL_MUSTLOCK(this->o_image)) SDL_UnlockSurface(this->o_image);
	}

	int redraw_target(void)
	{
		this->image = SDL_CreateTextureFromSurface(this->renderer, this->o_image);
		return (this->image) ? 0 : -1;
	}

	void new_image(SDL_Surface* image_s)
	{
		// Though a nullptr may be passed, the old data is still erased and dellocated.
		if (!image_s) SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Invalid file.", "The selected file could not be opened.", window);

		if (this->o_image) SDL_FreeSurface(this->o_image);
		if (this->image) SDL_DestroyTexture(this->image);
		this->o_image = image_s;
		this->refresh_size('I');
		this->redraw_target();
	}
};

int main(int argc, char* argv[])
{
    if (argc < 2) { printf("Image Lite; version: 1.2\nOpen images with this program to view them."); return 0; }  // Remember to change in the rc file as well.

    filesystem::path file_loc = string(argv[1]);
    if (!filesystem::exists(file_loc))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Invalid path.", "The selected file does not exist.", nullptr);
        return -1;
    }
    else if (!filesystem::is_regular_file(file_loc))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Invalid path.", "The selected path is not a file.", nullptr);
        return -2;
    }

    vector<filesystem::path> dir_files;
    size_t current_index = -1;
    for (auto entry : filesystem::directory_iterator(file_loc.parent_path()))
    {
        string ext = entry.path().extension().string();
        transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return tolower(c); });
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".ico")
        {
            dir_files.push_back(entry.path());
            if (entry.path() == file_loc) current_index = dir_files.size() - 1;
        }
    }

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);

    SDL_Window* window = SDL_CreateWindow("Image Lite", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 300, SDL_WINDOW_MAXIMIZED | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawColor(renderer, 0, 0,0, 255);

    SDL_Surface* image = IMG_Load(string(file_loc.string()).c_str());
    if (!image)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Invalid file.", "The selected file could not be opened.", window);
        SDL_Event quit;
        SDL_zero(quit);
        quit.type = SDL_QUIT;
        SDL_PushEvent(&quit);
    }

    int title_mode = 0;
    set_title(window, title_mode, file_loc);

    render_computation rcmp(window, renderer, image);

    SDL_Event event;
    while (SDL_WaitEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
            {
				if (rcmp.o_image) SDL_FreeSurface(rcmp.o_image);
				if (rcmp.image) SDL_DestroyTexture(rcmp.image);
                IMG_Quit();
                SDL_Quit();
                return 0;
            }
			case SDL_WINDOWEVENT: if (event.window.event == SDL_WINDOWEVENT_RESIZED) rcmp.refresh_size('W'); break;
            case SDL_KEYDOWN:
            {
                switch (event.key.keysym.sym)
                {
                    case SDLK_t:
                    {
                        ++title_mode;
                        title_mode %= 3;
                        set_title(window, title_mode, file_loc);
                        break;
                    }
                    case SDLK_b:
                    {
						if (!(event.key.keysym.mod & KMOD_SHIFT))
						{
							rcmp.light_mode = !rcmp.light_mode;
							if (rcmp.light_mode) SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
							else SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
						} else rcmp.apply_effect('b');
						break;
                    }
                    case SDLK_o: rcmp.original_size = !rcmp.original_size; break;
                    case SDLK_h: rcmp.flip_h = !rcmp.flip_h; break;
                    case SDLK_v: rcmp.flip_v = !rcmp.flip_v; break;
					case SDLK_c: rcmp.apply_effect('c'); break;
					case SDLK_i: rcmp.apply_effect('i'); break;
					case SDLK_g: if (event.key.keysym.mod & KMOD_SHIFT) rcmp.apply_effect('g'); break;
					case SDLK_r: if (event.key.keysym.mod & KMOD_SHIFT) rcmp.apply_effect('r'); break;
                    case SDLK_EQUALS: rcmp.scale(1); break;
                    case SDLK_MINUS: rcmp.scale(-1); break;
                    case SDLK_PAGEDOWN: rcmp.scale(-0.01 * (!(event.key.keysym.mod & KMOD_SHIFT) ? 10 : 1)); break;
                    case SDLK_PAGEUP: rcmp.scale(0.01 * (!(event.key.keysym.mod & KMOD_SHIFT) ? 10 : 1)); break;
                    case SDLK_0: rcmp.scale_reset(); break;
                    case SDLK_1: rcmp.zoom = 0.1 * (!(event.key.keysym.mod & KMOD_SHIFT) ? 10 : 1); break;
                    case SDLK_2: rcmp.zoom = 0.2 * (!(event.key.keysym.mod & KMOD_SHIFT) ? 10 : 1); break;
                    case SDLK_3: rcmp.zoom = 0.3 * (!(event.key.keysym.mod & KMOD_SHIFT) ? 10 : 1); break;
                    case SDLK_4: rcmp.zoom = 0.4 * (!(event.key.keysym.mod & KMOD_SHIFT) ? 10 : 1); break;
                    case SDLK_5: rcmp.zoom = 0.5 * (!(event.key.keysym.mod & KMOD_SHIFT) ? 10 : 1); break;
                    case SDLK_6: rcmp.zoom = 0.6 * (!(event.key.keysym.mod & KMOD_SHIFT) ? 10 : 1); break;
                    case SDLK_7: rcmp.zoom = 0.7 * (!(event.key.keysym.mod & KMOD_SHIFT) ? 10 : 1); break;
                    case SDLK_8: rcmp.zoom = 0.8 * (!(event.key.keysym.mod & KMOD_SHIFT) ? 10 : 1); break;
                    case SDLK_9: rcmp.zoom = 0.9 * (!(event.key.keysym.mod & KMOD_SHIFT) ? 10 : 1); break;
                    case SDLK_UP:
                    {
                        if (!(event.key.keysym.mod & KMOD_CTRL)) rcmp.y_offset -= 1 * (!(event.key.keysym.mod & KMOD_SHIFT) ? 10 : 1);
                        break;
                    }
                    case SDLK_DOWN:
                    {
                        if (!(event.key.keysym.mod & KMOD_CTRL)) rcmp.y_offset += 1 * (!(event.key.keysym.mod & KMOD_SHIFT) ? 10 : 1);
                        break;
                    }
                    case SDLK_LEFT:
                    {
                        if (event.key.keysym.mod & KMOD_CTRL)
                        {
                            if ((event.key.keysym.mod & KMOD_SHIFT)) rcmp.rotation -= 1;
                            else if ((event.key.keysym.mod & KMOD_ALT)) rcmp.rotation -= 45;
                            else rcmp.rotation -= 5;
                            rcmp.refresh_size('I');
                        }
                        else if (event.key.keysym.mod & KMOD_ALT)
                        {
                            if (!(dir_files.size() && current_index)) break;
                            --current_index;
                            file_loc = dir_files[current_index];
                            rcmp.new_image(IMG_Load(string(file_loc.string()).c_str()));
                            set_title(window, title_mode, file_loc);
                        }
                        else rcmp.x_offset -= 1 * (!(event.key.keysym.mod & KMOD_SHIFT) ? 10 : 1);
                        break;
                    }
                    case SDLK_RIGHT:
                    {
                        if (event.key.keysym.mod & KMOD_CTRL)
                        {
                            if ((event.key.keysym.mod & KMOD_SHIFT)) rcmp.rotation += 1;
                            else if ((event.key.keysym.mod & KMOD_ALT)) rcmp.rotation += 45;
                            else rcmp.rotation += 5;
                            rcmp.refresh_size('I');
                        }
                        else if (event.key.keysym.mod & KMOD_ALT)
                        {
                            if (!dir_files.size() || (current_index == dir_files.size() - 1)) break;
                            ++current_index;
                            file_loc = dir_files[current_index];
                            rcmp.new_image(IMG_Load(string(file_loc.string()).c_str()));
                            set_title(window, title_mode, file_loc);
                        }
                        else rcmp.x_offset += 1 * (!(event.key.keysym.mod & KMOD_SHIFT) ? 10 : 1);
                        break;
                    }
                    case SDLK_F11: rcmp.toggle_fullscreen(); break;
                    case SDLK_ESCAPE:
                    case SDLK_STOP:
                    case SDLK_CANCEL:
                    {
                        SDL_Event quit;
                        SDL_zero(quit);
                        quit.type = SDL_QUIT;
                        SDL_PushEvent(&quit);
                        break;
                    }
                }
                break;
            }
            case SDL_MOUSEWHEEL:
            {
                rcmp.x_offset += 50 * event.wheel.x;
                rcmp.y_offset -= 50 * event.wheel.y;        // x is + but y is -; that's how it works.
                break;
            }
        }

        rcmp.refresh_screen();
    }

    return 0;
}


// UPDATE ORIGINAL VIEW.
