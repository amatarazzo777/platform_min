/**
\author Anthony Matarazzo
\file viewManager.hpp
\date 11/19/19
\version 1.0
\brief Header file that implements the document object model interface.
The attributes, Element base class, and document entities are defined within
the file. The enumeration values for object options as well as the event class
are defined here. Within this file, several preprocessor macros exist that
simplify and document the code base. Based upon the environment of the compiler,
several platform specific header files are included. However all of the
platform OS code is only coded within the platform object. The system exists
within the viewManager namespace.
*/
#pragma once

/**
\addtogroup Library Build Options
\brief Library Options
\details These options provide the selection to configure selection
options when compiling the source.
@{
*/

#define DEFAULT_TEXTFACE "arial"
#define DEFAULT_TEXTSIZE 12
#define DEFAULT_TEXTCOLOR 0



/**
\def USE_DIRECT
\brief The system will use XCB for screen output
*/
#define USE_DIRECT_SCREEN_OUTPUT
#define USE_SDL_SCREEN_OUTPUT


/**
\def USE_FREETYPE
\brief 1The system will be configured to use the freetype library intrinsically.
*/
#define USE_FREETYPE


/**
\def USE_GREYSCALE_ANTIALIAS
\brief Use the freetype greyscale rendering. The Option is only for use with the
inline render. Use this option or the USE_LCD_FILTER. One one should be defined.
*/
//#define USE_FREETYPE_GREYSCALE_ANTIALIAS

/**
\def USE_LCD_FILTER
\brief The system must be configured to use the inline renderer. This uses the
lcd filtering mode of the freetype glyph library. The option is exclusive
against the USE_GREYSCALE_ANTIALIAS option. One one should be defined.
*/
#define USE_FREETYPE_LCD_FILTER


/**
\def USE_IMAGE_MAGICK
\brief the ImageMagick library is used for image
 * loading and processing.

*/
#define USE_IMAGE_MAGICK

/**
\def USE_STB_IMAGE
\brief the stb_image file loader is used.
*/
//#define USE_STB_IMAGE

/**
\def USE_CHROMIUM_EMBEDDED_FRAMEWORK
\brief The system will be configured to use the CEF system.
*/
//#define USE_CHROMIUM_EMBEDDED_FRAMEWORK

/** @} */

#include <algorithm>
#include <any>
#include <array>
#include <cstdint>

#if defined(_WIN64)
typedef unsigned char u_int8_t;
#endif

#include <cctype>
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <assert.h>

/*************************************
OS SPECIFIC HEADERS
*************************************/

#if defined(__linux__)
#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/Xlib-xcb.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <fontconfig/fontconfig.h>
#include <xcb/shm.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_keysyms.h>

#elif defined(_WIN64)
// Windows Header Files:
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

// auto linking of direct x
#pragma comment(lib, "d2d1.lib")

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

#endif

#ifdef USE_FREETYPE
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_SIZES_H
#endif

#ifdef USE_IMAGE_MAGICK
#include <Magick++.h>

#endif // image processing

namespace uxdevice {

class event;

/**
\enum eventType
\brief the eventType enumeration contains a sequenced value for all of the
events that can be dispatched by the system.
*/
enum class eventType : uint8_t {
  paint,
  focus,
  blur,
  resize,
  keydown,
  keyup,
  keypress,
  mouseenter,
  mousemove,
  mousedown,
  mouseup,
  click,
  dblclick,
  contextmenu,
  wheel,
  mouseleave
};

/// \typedef eventHandler is used to note and declare a lambda function for
/// the specified event.
typedef std::function<void(const event &et)> eventHandler;

/**
\class event

\brief the event class provides the communication between the event system and
the caller. There is one event class for all of the distinct events. Simply
different constructors are selected based upon the necessity of information
given within the parameters.
*/
using event = class event {
public:
  event(const eventType &et) {
    evtType = et;
    bVirtualKey = false;
  }
  event(const eventType &et, const char &k) {
    evtType = et;
    key = k;
    bVirtualKey = false;
  }
  event(const eventType &et, const unsigned int &vk) {
    evtType = et;
    virtualKey = vk;
    bVirtualKey = true;
  }

  event(const eventType &et, const short &mx, const short &my,
        const short &mb_dis) {
    evtType = et;
    mousex = mx;
    mousey = my;
    if (et == eventType::wheel)
      wheelDistance = mb_dis;
    else
      mouseButton = static_cast<char>(mb_dis);
    bVirtualKey = false;
  }
  event(const eventType &et, const short &w, const short &h) {
    evtType = et;
    width = w;
    height = h;
    mousex = w;
    mousey = h;
    bVirtualKey = false;
  }
  event(const eventType &et, const short &distance) {
    evtType = et;
    wheelDistance = distance;
    bVirtualKey = false;
  }
  ~event(){};

public:
  eventType evtType;
  bool bVirtualKey;
  char key;
  unsigned int virtualKey;
  std::wstring unicodeKeys;
  short mousex;
  short mousey;
  char mouseButton;
  short width;
  short height;
  short wheelDistance;
};

/**
 \details
*/

using rectangle = class rectangle {
public:
  rectangle(int _x1, int _y1, int _x2, int _y2)
      : x1(_x1), y1(_y1), x2(_x2), y2(_y2) {}
  int x1;
  int y1;
  int x2;
  int y2;
};

using stringData = class stringData {
public:
  std::shared_ptr<std::string> data;
  bool bWordBreaks = true;
};

using imageData = class imageData {
public:
  imageData(std::shared_ptr<int> _width, std::shared_ptr<int> _height,
            std::shared_ptr<std::vector<u_int8_t>> _data);
  imageData(std::shared_ptr<std::string> _fileName);

#if defined(USE_STB_IMAGE)
  std::shared_ptr<int> width;
  std::shared_ptr<int> height;
  std::shared_ptr<std::vector<u_int8_t>> data;

#elif defined(USE_IMAGE_MAGICK)
  std::shared_ptr<Magick::Image> data;
#endif

  std::shared_ptr<std::string> fileName;
};

using textFace = class textFace {
public:
  std::shared_ptr<std::string> data;
  std::shared_ptr<int> pointSize;
};

using textColor = class textColor {
public:
  std::shared_ptr<unsigned int> data;
};
using textAlignment = class textAlignment {
public:
  std::shared_ptr<char> data;
};

using targetArea = class targetArea {
public:
  std::shared_ptr<rectangle> data;
};

using catchEvent = class catchEvent {
public:
  std::shared_ptr<eventHandler> data;
};
using drawText = class drawText {
public:
  std::shared_ptr<std::size_t> beginIndex;
  std::shared_ptr<std::size_t> endIndex;
};
using drawImage = class drawImage {
public:
  std::shared_ptr<rectangle> src;
};

typedef std::variant<stringData, imageData, textFace, textColor, textAlignment,
                     targetArea, catchEvent, drawText, drawImage>
    displayListType;

/**
\internal
\class platform
\brief The platform contains logic to connect the document object model to the
local operating system.
*/
class platform {
public:
  platform(const eventHandler &evtDispatcher);
  ~platform();
  void openWindow(const std::string &sWindowTitle, const unsigned short width,
                  const unsigned short height);
  void closeWindow(void);

  std::vector<displayListType> &data(void);
  void dirty(std::size_t idx) { m_dirty.push_back(idx); }
  int pixelWidth(std::size_t idx) { return 0; }
  int pixelHeight(std::size_t idx) { return 0; }
  void render();
  void processEvents(void);
  void dispatchEvent(const event &e);
  int measureTextWidth(const std::string &sTextFace, const int pointSize,
                       const std::string &s);
  int measureFaceHeight(const std::string &sTextFace, const int pointSize);

private:
  std::vector<displayListType> DL;
  std::vector<std::size_t> m_dirty;
  std::shared_ptr<rectangle> m_targetArea;
  std::shared_ptr<std::string> m_stringData;

#if defined(USE_STB_IMAGE)
  std::shared_ptr<std::vector<u_int8_t>> m_imageData;
  std::shared_ptr<int> m_width;
  std::shared_ptr<int> m_height;

#elif defined(USE_IMAGE_MAGICK)
  std::shared_ptr<Magick::Image> m_image;

#endif

  std::shared_ptr<std::string> m_textFace;
  std::shared_ptr<int> m_pointSize;

#if defined(USE_DIRECT_SCREEN_OUTPUT)
  std::shared_ptr<unsigned int> m_textColor;
  unsigned char m_textColorR;
  unsigned char m_textColorG;
  unsigned char m_textColorB;
  bool m_bProcessedOnce;

#elif defined(USE_IMAGE_MAGICK)
  std::shared_ptr<Magick::Color> m_textColor;
#endif // defined

  std::shared_ptr<char> m_textAlignment;

  int m_xpos;
  int m_ypos;

private:
  void drawCaret(const int x, const int y, const int h);

  inline void putPixel(const int x, const int y, const unsigned int color);
  inline unsigned int getPixel(const int x, const int y);


#if defined(USE_FREETYPE)
  void activateTextFace(void);
  void renderText(const std::size_t &beginIndex, const std::size_t &endIndex);
  int renderChar(const char c);
  inline FTC_FaceID getFaceID(std::string sTextFace);
#endif // defined

  void renderImage(const rectangle &src);
  void messageLoop(void);
  void test(int x, int y);

  void flip(void);
  void resize(const int w, const int h);
  void clear(void);

#if defined(USE_FREETYPE)
  std::string getFontFilename(const std::string &sTextFace);
#endif // defined


#if defined(USE_DIRECT_SCREEN_OUTPUT) && defined(__linux__)

#elif defined(USE_DIRECT_SCREEN_OUTPUT) && defined(_WIN64)
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam,
                                  LPARAM lParam);
  bool initializeVideo(void);
  void terminateVideo(void);
#endif

#if defined(USE_FREETYPE)
  typedef struct {
    std::string filePath;
    int index;
  } faceCacheStruct;

  static FT_Error faceRequestor(FTC_FaceID face_id, FT_Library library,
                                FT_Pointer request_data, FT_Face *aface);

#endif

private:
#if defined(USE_DIRECT_SCREEN_OUTPUT) && defined(__linux__)
  Display *m_xdisplay;
  xcb_connection_t *m_connection;
  xcb_screen_t *m_screen;
  xcb_drawable_t m_window;
  xcb_gcontext_t m_graphics;
  xcb_pixmap_t m_pix;
  xcb_shm_segment_info_t m_info;

  // xcb -- keyboard
  xcb_key_symbols_t *m_syms;
  uint32_t m_foreground;
  u_int8_t *m_screenMemoryBuffer;

#elif defined(USE_DIRECT_SCREEN_OUTPUT) && defined(_WIN64)
  HWND m_hwnd;

  ID2D1Factory *m_pD2DFactory;
  ID2D1HwndRenderTarget *m_pRenderTarget;
  ID2D1Bitmap *m_pBitmap;

#endif

  int fontScale;

#if defined(USE_DIRECT_SCREEN_OUTPUT) && !defined(USE_IMAGE_MAGICK)
  std::vector<u_int8_t> m_offscreenBuffer;


#elif defined(USE_IMAGE_MAGICK)
  Magick::Image m_offscreenImage;
  Magick::Quantum *m_offscreenBuffer;


#endif

private:
  eventHandler fnEvents;

  unsigned short _w;
  unsigned short _h;

#if defined(USE_FREETYPE)
  FT_Library m_freeType;
  FTC_Manager m_cacheManager;

#if defined(USE_FREETYPE_LCD_FILTER)
  FTC_ImageCache m_imageCache;

#elif defined(USE_FREETYPE_GREYSCALE_ANTIALIAS)
  FTC_SBitCache m_bitCache;
#endif

  FTC_CMapCache m_cmapCache;
  std::unordered_map<std::string, faceCacheStruct> m_faceCache;
  typedef std::unordered_map<std::string, faceCacheStruct>::iterator
      faceCacheIterator;

  FT_Error m_error;
  FTC_ScalerRec m_scaler;

  FT_Size m_sizeFace;
  FT_UInt m_glyph_index = 0;
  FT_UInt m_previous_index = 0;
  FTC_FaceID m_faceID;
  int m_faceHeight;
#endif

private:
  std::vector<eventHandler> onfocus;
  std::vector<eventHandler> onblur;
  std::vector<eventHandler> onresize;
  std::vector<eventHandler> onkeydown;
  std::vector<eventHandler> onkeyup;
  std::vector<eventHandler> onkeypress;
  std::vector<eventHandler> onmouseenter;
  std::vector<eventHandler> onmouseleave;
  std::vector<eventHandler> onmousemove;
  std::vector<eventHandler> onmousedown;
  std::vector<eventHandler> onmouseup;
  std::vector<eventHandler> onclick;
  std::vector<eventHandler> ondblclick;
  std::vector<eventHandler> oncontextmenu;
  std::vector<eventHandler> onwheel;

private:
  std::vector<eventHandler> &getEventVector(eventType evtType);
};

}; // namespace uxdevice
