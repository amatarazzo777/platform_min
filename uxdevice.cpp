/**
\file uxdevice.cpp

\author Anthony Matarazzo

\date 3/26/20
\version 1.0
*/

/**
\brief rendering and platform services.

*/
#include "uxdevice.hpp"

#ifdef USE_STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"
#endif // USE_STB_IMAGE

#include <sys/types.h>

using namespace std;
using namespace uxdevice;

/**
\internal
\brief The routine iterates the display list moving
parameters to the class member communication areas.
If processing is requested, the function operation
is
invoked.
*/
void uxdevice::platform::render(void) {

#if defined(USE_IMAGE_MAGICK)
  m_offscreenImage.modifyImage();
  Magick::Pixels view(m_offscreenImage);
  m_offscreenBuffer = view.get(0,0,m_offscreenImage.columns(),m_offscreenImage.rows());
#endif // defined

  for (auto &n : DL) {
    if (holds_alternative<stringData>(n)) {
      m_stringData = get<stringData>(n).data;

    } else if (holds_alternative<imageData>(n)) {

#if defined(USE_STB_IMAGE)
      m_imageData = get<imageData>(n).data;
      m_width = get<imageData>(n).width;
      m_height = get<imageData>(n).height;

#elif defined(USE_IMAGE_MAGICK)
      m_image = get<imageData>(n).data;

#endif // USE_IMAGE_MAGICK

    } else if (holds_alternative<textFace>(n)) {
      m_textFace = get<textFace>(n).data;
      m_pointSize = get<textFace>(n).pointSize;
      activateTextFace();

    } else if (holds_alternative<textColor>(n)) {
      m_textColor = get<textColor>(n).data;
      // get color components of the foreground color used later.
      m_textColorR = *m_textColor >> 16;
      m_textColorG = *m_textColor >> 8;
      m_textColorB = *m_textColor;

    } else if (holds_alternative<textAlignment>(n)) {
      m_textAlignment = get<textAlignment>(n).data;

    } else if (holds_alternative<targetArea>(n)) {
      m_targetArea = get<targetArea>(n).data;

    } else if (holds_alternative<drawText>(n)) {
      const size_t &beginIndex = *get<drawText>(n).beginIndex;
      const size_t &endIndex = *get<drawText>(n).endIndex;

      renderText(beginIndex, endIndex);

    } else if (holds_alternative<drawImage>(n)) {
      const rectangle &src = *get<drawImage>(n).src;
      renderImage(src);
    }
  }

#if defined(USE_IMAGE_MAGICK)
  view.sync();
#endif // defined
}

/**
\internal
\brief a simple test of the pointer and shared memory .
*/
// change data on mouse move
void uxdevice::platform::test(int x, int y) {
return;
  for (auto &n : DL) {
    // visit based upon type, from std c++ reference example
    if (holds_alternative<targetArea>(n)) {
      m_targetArea = get<targetArea>(n).data;
      m_targetArea->x1 = x;
      m_targetArea->y1 = y;
      m_targetArea->x2 = x + 600;
      m_targetArea->y2 = y + 600;
    }
  }
}

/*
\brief the dispatch routine is invoked by the messageLoop.
If default
 * handling is to be supplied, the method invokes the
necessary operation.

*/
void uxdevice::platform::dispatchEvent(const event &evt) {
  switch (evt.evtType) {
  case eventType::paint:
    clear();
    render();
    flip();
    break;
  case eventType::resize:
    resize(evt.width, evt.height);
    //    render();
    break;
  case eventType::keydown: {

  } break;
  case eventType::keyup: {

  } break;
  case eventType::keypress: {

  } break;
  case eventType::mousemove:
       // test(evt.mousex, evt.mousey);
    //   dispatchEvent(event{eventType::paint});
    break;
  case eventType::mousedown:
    break;
  case eventType::mouseup:
    if (evt.mouseButton == 1)
      fontScale++;
    else
      fontScale--;
    if (fontScale < 5)
      fontScale = 5;
    if (fontScale > 100)
      fontScale = 100;

    dispatchEvent(event{eventType::paint});
    break;
  case eventType::wheel:
    if (evt.wheelDistance > 0)
      fontScale += 1;
    else
      fontScale -= 1;
    if (fontScale < 5)
      fontScale = 5;
    if (fontScale > 100)
      fontScale = 100;
    dispatchEvent(event{eventType::paint});
    break;
  }
/* these events do not come from the platform. However,
they are spawned from conditions based upon the platform events.
*/
#if 0
eventType::focus
eventType::blur
eventType::mouseenter
eventType::click
eventType::dblclick
eventType::contextmenu
eventType::mouseleave
#endif
}
/**
\internal
\brief The entry point that processes messages from the operating
system application level services. Typically on Linux this is a
coupling of xcb and keysyms library for keyboard. Previous
incarnations of technology such as this typically used xserver.
However, XCB is the newer form. Primarily looking at the code of such
programs as vlc, the routine simply places pixels into the memory
buffer. while on windows the direct x library is used in combination
with windows message queue processing.
*/
void uxdevice::platform::processEvents(void) {
  // setup the event dispatcher
  eventHandler ev = std::bind(&uxdevice::platform::dispatchEvent, this,
                              std::placeholders::_1);

  messageLoop();
}

/**
\internal

\brief The function maps the event id to the appropriate vector.
This is kept statically here for resource management.

\param eventType evtType
*/
vector<eventHandler> &uxdevice::platform::getEventVector(eventType evtType) {
  static unordered_map<eventType, vector<eventHandler> &> eventTypeMap = {
      {eventType::focus, onfocus},
      {eventType::blur, onblur},
      {eventType::resize, onresize},
      {eventType::keydown, onkeydown},
      {eventType::keyup, onkeyup},
      {eventType::keypress, onkeypress},
      {eventType::mouseenter, onmouseenter},
      {eventType::mouseleave, onmouseleave},
      {eventType::mousemove, onmousemove},
      {eventType::mousedown, onmousedown},
      {eventType::mouseup, onmouseup},
      {eventType::click, onclick},
      {eventType::dblclick, ondblclick},
      {eventType::contextmenu, oncontextmenu},
      {eventType::wheel, onwheel}};
  auto it = eventTypeMap.find(evtType);
  return it->second;
}
/**
\internal
\brief
The function will return the address of a std::function for the purposes
of equality testing. Function from
https://stackoverflow.com/questions/20833453/comparing-stdfunctions-for-equality

*/
template <typename T, typename... U>
size_t getAddress(std::function<T(U...)> f) {
  typedef T(fnType)(U...);
  fnType **fnPointer = f.template target<fnType *>();
  return (size_t)*fnPointer;
}

#if 0
/**

\brief The function is invoked when an event occurrs. Normally this occurs
from the platform device. However, this may be invoked by the soft
generation of events.

*/
void uxdevice::platform::dispatch(const event &e) {
  auto &v = getEventVector(e.evtType);
  for (auto &fn : v)
    fn(e);
}
#endif

/**
  \internal
  \brief constructor for the platform object. The platform object is coded
  such that each of the operating systems supported is encapsulated within
  preprocessor blocks.

  \param eventHandler evtDispatcher the dispatcher routine which connects
  the platform to the object model system. \param unsigned short width -
  window size. \param unsigned short height - window size.
*/
uxdevice::platform::platform(const eventHandler &evtDispatcher) {
  fnEvents = evtDispatcher;

  fontScale = 0;

// initialize private members
#if defined(__linux__)
  m_connection = nullptr;
  m_screen = nullptr;
  m_window = 0;
  m_syms = nullptr;
  m_foreground = 0;

#elif defined(_WIN64)

  m_hwnd = 0x00;
  CoInitialize(NULL);

#endif

#if defined(USE_FREETYPE)
  const char *errText = "The freetype library could not be initialized.";

  FT_Error error;

  // init the freetype library
  error = FT_Init_FreeType(&m_freeType);
  if (error)
    throw std::runtime_error(errText);

  // initalize the freetype cache
  error = FTC_Manager_New(m_freeType, 0, 0, 0, &faceRequestor, NULL,
                          &m_cacheManager);
  if (error)
    throw std::runtime_error(errText);

#ifdef USE_FREETYPE_GREYSCALE_ANTIALIAS
  // initialize the bitmap cache
  error = FTC_SBitCache_New(m_cacheManager, &m_bitCache);
  if (error)
    throw std::runtime_error(errText);

#elif defined USE_FREETYPE_LCD_FILTER

  // initialize the image cache
  error = FTC_ImageCache_New(m_cacheManager, &m_imageCache);
  if (error)
    throw std::runtime_error(errText);

#endif

  error = FTC_CMapCache_New(m_cacheManager, &m_cmapCache);
  if (error)
    throw std::runtime_error(errText);

#endif

#ifdef USE_IMAGE_MAGICK
  Magick::InitializeMagick("");

#endif
}
/**
  \internal
  \brief terminates the xserver connection
  and frees resources.
*/
uxdevice::platform::~platform() {
// Freetype can be used for windows or linux
#ifdef USE_FREETYPE
  FTC_Manager_Done(m_cacheManager);
  FT_Done_FreeType(m_freeType);
#endif

#if defined(USE_DIRECT_SCREEN_OUTPUT) && defined(__linux__)
  xcb_shm_detach(m_connection, m_info.shmseg);
  shmdt(m_info.shmaddr);

  xcb_free_pixmap(m_connection, m_pix);
  xcb_free_gc(m_connection, m_foreground);
  xcb_key_symbols_free(m_syms);

  xcb_destroy_window(m_connection, m_window);
  xcb_disconnect(m_connection);
  XCloseDisplay(m_xdisplay);

#elif defined(USE_DIRECT_SCREEN_OUTPUT) && defined(_WIN64)
  CoUninitialize();

#endif

}
/**
  \internal
  \brief opens a window on the target OS

*/
void uxdevice::platform::openWindow(const std::string &sWindowTitle,
                                    const unsigned short width,
                                    const unsigned short height) {
  _w = width;
  _h = height;

#if defined(USE_IMAGE_MAGICK)
  m_offscreenImage.size(Magick::Geometry(_w,_h));
#endif // defined

#if defined(USE_DIRECT_SCREEN_OUTPUT) && defined(__linux__)
  // this open provide interoperability between xcb and xwindows
  // this is used here because of the necessity of key mapping.
  m_xdisplay = XOpenDisplay(nullptr);

  /* get the connection to the X server */
  m_connection = XGetXCBConnection(m_xdisplay);

  /* Get the first screen */
  m_screen = xcb_setup_roots_iterator(xcb_get_setup(m_connection)).data;
  m_syms = xcb_key_symbols_alloc(m_connection);

  /* Create black (foreground) graphic context */
  m_window = m_screen->root;
  m_graphics = xcb_generate_id(m_connection);
  uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
  uint32_t values[2] = {m_screen->black_pixel, 0};
  xcb_create_gc(m_connection, m_graphics, m_window, mask, values);

  /* Create a window */
  m_window = xcb_generate_id(m_connection);
  mask = XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
  values[0] = 0;
  values[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS |
              XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_POINTER_MOTION |
              XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_BUTTON_PRESS |
              XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

  xcb_create_window(
      m_connection, XCB_COPY_FROM_PARENT, m_window, m_screen->root, 0, 0,
      static_cast<unsigned short>(_w), static_cast<unsigned short>(_h), 10,
      XCB_WINDOW_CLASS_INPUT_OUTPUT, m_screen->root_visual, mask, values);
  // set window title
  xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_window,
                      XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, sWindowTitle.size(),
                      sWindowTitle.data());

  // create offscreen bitmap
  resize(_w, _h);
  clear();
  render();
  flip();

  /* Map the window on the screen and flush*/
  xcb_map_window(m_connection, m_window);
  xcb_flush(m_connection);

  return;

#elif defined(USE_DIRECT_SCREEN_OUTPUT) && defined(_WIN64)

  // Register the window class.
  WNDCLASSEX wcex = {sizeof(WNDCLASSEX)};
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = &uxdevice::platform::WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = sizeof(LONG_PTR);
  wcex.hInstance = HINST_THISCOMPONENT;
  wcex.hbrBackground = NULL;
  wcex.lpszMenuName = NULL;
  wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
  wcex.lpszClassName = "viewManagerApp";
  RegisterClassEx(&wcex);
  // Create the window.
  m_hwnd =
      CreateWindow("viewManagerApp", sWindowTitle.data(), WS_OVERLAPPEDWINDOW,
                   CW_USEDEFAULT, CW_USEDEFAULT, static_cast<UINT>(_w),
                   static_cast<UINT>(_h), NULL, NULL, HINST_THISCOMPONENT, 0L);

  SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (long long)this);

  if (!initializeVideo())
    throw std::runtime_error("Could not initalize direct x video subsystem.");

  // create offscreen bitmap
  resize(_w, _h);

  ShowWindow(m_hwnd, SW_SHOWNORMAL);
  UpdateWindow(m_hwnd);

#endif
}

/**
  \internal
  \brief Initalize the direct 3 video system.

  Orginal code from
*/
#if defined(USE_DIRECT_SCREEN_OUTPUT) &&  defined(_WIN64)
bool uxdevice::platform::initializeVideo() {
  HRESULT hr;

  // Create a Direct2D factory.
  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2DFactory);

  RECT rc;
  GetClientRect(m_hwnd, &rc);

  D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

  // Create a Direct2D render target.
  hr = m_pD2DFactory->CreateHwndRenderTarget(
      D2D1::RenderTargetProperties(),
      D2D1::HwndRenderTargetProperties(m_hwnd, size), &m_pRenderTarget);
  return true;
}

/**
  \brief terminateVideo
  \description the routine frees the resources of directx.
*/
void uxdevice::platform::terminateVideo(void) {
  m_pD2DFactory->Release();
  m_pRenderTarget->Release();
}

#endif

/**
  \internal
  \brief closes a window on the target OS


*/
void uxdevice::platform::closeWindow(void) {
#if defined(USE_DIRECT_SCREEN_OUTPUT) && defined(__linux__)

#elif defined(USE_DIRECT_SCREEN_OUTPUT) && defined(_WIN64)

#endif
}

/**
\brief A reference to the internal display list is returned.
*

*/
std::vector<displayListType> &uxdevice::platform::data(void) { return (DL); }

#if defined(USE_DIRECT_SCREEN_OUTPUT) && defined(_WIN64)

/**
\internal
\brief The default window message processor for the application.
This is the version of the Microsoft Windows operating system.

*/
LRESULT CALLBACK uxdevice::platform::WndProc(HWND hwnd, UINT message,
                                             WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;
  bool handled = false;
  /** get the platform objext which is stored within the user data of the
   window. this is necessary as the wndproc for the windows operating system
   is called from an external library. The routine needs to be a static
   implementation which is not directly locate within the class.
  */
  LONG_PTR lpUserData = GetWindowLongPtr(hwnd, GWLP_USERDATA);
  platform *platformInstance = (platform *)lpUserData;
  switch (message) {
  case WM_SIZE:
    platformInstance->dispatchEvent(event{eventType::resize,
                                          static_cast<short>(LOWORD(lParam)),
                                          static_cast<short>(HIWORD(lParam))});
    result = 0;
    handled = true;
    break;
  case WM_KEYDOWN: {
    UINT scandCode = (lParam >> 8) & 0xFFFFFF00;
    platformInstance->dispatchEvent(
        event{eventType::keydown, (unsigned int)wParam});
    handled = true;
  } break;
  case WM_KEYUP: {
    UINT scandCode = (lParam >> 8) & 0xFFFFFF00;
    platformInstance->dispatchEvent(
        event{eventType::keyup, (unsigned int)wParam});
    handled = true;
  } break;
  case WM_CHAR: {
    // filter out some of the control keys that
    // slip through such as the back and tab keys
    if (wParam > 27) {
      WCHAR tmp[2];
      tmp[0] = wParam;
      tmp[1] = 0x00;
      char ch = wParam;
      platformInstance->dispatchEvent(event{eventType::keypress, ch});
      handled = true;
    }
  } break;
  case WM_LBUTTONDOWN:
    platformInstance->dispatchEvent(
        event{eventType::mousedown, static_cast<short>(LOWORD(lParam)),
              static_cast<short>(HIWORD(lParam)), 1});
    handled = true;
    break;
  case WM_LBUTTONUP:
    platformInstance->dispatchEvent(
        event{eventType::mouseup, static_cast<short>(LOWORD(lParam)),
              static_cast<short>(HIWORD(lParam)), 1});
    handled = true;
    break;
  case WM_MBUTTONDOWN:
    platformInstance->dispatchEvent(
        event{eventType::mousedown, static_cast<short>(LOWORD(lParam)),
              static_cast<short>(HIWORD(lParam)), 2});
    handled = true;
    break;
  case WM_MBUTTONUP:
    platformInstance->dispatchEvent(
        event{eventType::mouseup, static_cast<short>(LOWORD(lParam)),
              static_cast<short>(HIWORD(lParam)), 2});
    handled = true;
    break;
  case WM_RBUTTONDOWN:
    platformInstance->dispatchEvent(
        event{eventType::mousedown, static_cast<short>(LOWORD(lParam)),
              static_cast<short>(HIWORD(lParam)), 3});
    handled = true;
    break;
  case WM_RBUTTONUP:
    platformInstance->dispatchEvent(
        event{eventType::mouseup, static_cast<short>(LOWORD(lParam)),
              static_cast<short>(HIWORD(lParam)), 3});
    handled = true;
    break;
  case WM_MOUSEMOVE:
    platformInstance->dispatchEvent(event{eventType::mousemove,
                                          static_cast<short>(LOWORD(lParam)),
                                          static_cast<short>(HIWORD(lParam))});
    result = 0;
    handled = true;
    break;
  case WM_MOUSEWHEEL: {
    platformInstance->dispatchEvent(event{
        eventType::wheel, static_cast<short>(LOWORD(lParam)),
        static_cast<short>(HIWORD(lParam)), GET_WHEEL_DELTA_WPARAM(wParam)});
    handled = true;
  } break;
  case WM_DISPLAYCHANGE:
    InvalidateRect(hwnd, NULL, FALSE);
    result = 0;
    handled = true;
    break;
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    platformInstance->dispatchEvent(event{eventType::paint});
    EndPaint(hwnd, &ps);
    ValidateRect(hwnd, NULL);
    result = 0;
    handled = true;
  } break;
  case WM_DESTROY:
    PostQuitMessage(0);
    result = 1;
    handled = true;
    break;
  }
  if (!handled)
    result = DefWindowProc(hwnd, message, wParam, lParam);
  return result;
}
#endif

/**
\internal
\brief the routine handles the message processing for the specific
operating system. The function is called from processEvents.

*/
void uxdevice::platform::messageLoop(void) {
#if defined(USE_DIRECT_SCREEN_OUTPUT) && defined(__linux__)
  xcb_generic_event_t *xcbEvent;
  bool bRequestResize = false;
  short int newWidth;
  short int newHeight;

  while ((xcbEvent = xcb_wait_for_event(m_connection))) {
    switch (xcbEvent->response_type & ~0x80) {
    case XCB_MOTION_NOTIFY: {
      xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)xcbEvent;
      dispatchEvent(event{
          eventType::mousemove,
          motion->event_x,
          motion->event_y,
      });
    } break;
    case XCB_BUTTON_PRESS: {
      xcb_button_press_event_t *bp = (xcb_button_press_event_t *)xcbEvent;
      if (bp->detail == XCB_BUTTON_INDEX_4 ||
          bp->detail == XCB_BUTTON_INDEX_5) {
        dispatchEvent(event{eventType::wheel, bp->event_x, bp->event_y,
                            bp->detail == XCB_BUTTON_INDEX_4 ? 1 : -1});

      } else {
        dispatchEvent(
            event{eventType::mousedown, bp->event_x, bp->event_y, bp->detail});
      }
    } break;
    case XCB_BUTTON_RELEASE: {
      xcb_button_release_event_t *br = (xcb_button_release_event_t *)xcbEvent;
      // ignore button 4 and 5 which are wheel events.
      if (br->detail != XCB_BUTTON_INDEX_4 && br->detail != XCB_BUTTON_INDEX_5)
        dispatchEvent(
            event{eventType::mouseup, br->event_x, br->event_y, br->detail});
    } break;
    case XCB_KEY_PRESS: {
      xcb_key_press_event_t *kp = (xcb_key_press_event_t *)xcbEvent;
      xcb_keysym_t sym = xcb_key_press_lookup_keysym(m_syms, kp, 0);
      if (sym < 0x99) {
        XKeyEvent keyEvent;
        keyEvent.display = m_xdisplay;
        keyEvent.keycode = kp->detail;
        keyEvent.state = kp->state;
        std::array<char, 16> buf{};
        if (XLookupString(&keyEvent, buf.data(), buf.size(), nullptr, nullptr))
          dispatchEvent(event{eventType::keypress, (char)buf[0]});
      } else {
        dispatchEvent(event{eventType::keydown, sym});
      }
    } break;
    case XCB_KEY_RELEASE: {
      xcb_key_release_event_t *kr = (xcb_key_release_event_t *)xcbEvent;
      xcb_keysym_t sym = xcb_key_press_lookup_keysym(m_syms, kr, 0);
      dispatchEvent(event{eventType::keyup, sym});
    } break;
    case XCB_EXPOSE: {
      if (bRequestResize) {
        dispatchEvent(event{eventType::resize, newWidth, newHeight});
        bRequestResize = false;
      }
      dispatchEvent(event{eventType::paint});
    } break;
    case XCB_CONFIGURE_NOTIFY: {
      const xcb_configure_notify_event_t *cfgEvent =
          (const xcb_configure_notify_event_t *)xcbEvent;
      if (cfgEvent->window == m_window) {
        newWidth = cfgEvent->width;
        newHeight = cfgEvent->height;
        if ((newWidth != _w || newHeight != _h) && (newWidth > 0) &&
            (newHeight > 0)) {
          bRequestResize = true;
        }
      }
    }
    }
    free(xcbEvent);
  }
#elif defined(USE_DIRECT_SCREEN_OUTPUT) && defined(_WIN64)
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
#endif
}

#if defined(USE_FREETYPE)
/**
\internal
\brief The faceRequestor is a callback routine that provides
creation of a face object. The parameter face_id is a pointer
that is named as user information by the cache system.

\param FTC_FaceID face_id the user generated index
\param FT_Library library handle to the free type library
\param FT_Pointer request_data unused
\param FT_Face *aface the newly ycreated fash object.
*/
FT_Error uxdevice::platform::faceRequestor(FTC_FaceID face_id,
                                           FT_Library library,
                                           FT_Pointer request_data,
                                           FT_Face *aface) {
  FT_Error error;
  faceCacheStruct *fID = static_cast<faceCacheStruct *>(face_id);
  error = FT_New_Face(library, fID->filePath.data(), 0, aface);

  // we want to use unicode
  error = FT_Select_Charmap(*aface, FT_ENCODING_UNICODE);

  return error;
}
#endif

/**
\internal
\brief The function provides the building and location of a textFace name
The function independently works on linux vs. windows. The linux is much
more advanced in that it uses the fontconfig api. This api provides for
family matching as a browser would incorporate. Whereas the windows portion
uses the registry access and simply compares a string.

The function comes from the following source:
https://stackoverflow.com/questions/10542832/how-to-use-fontconfig-to-get-font-list-c-c
https://stackoverflow.com/questions/3954223/platform-independent-way-to-get-font-directory

\param sTextFace

*/

#if defined(USE_FREETYPE)

std::string uxdevice::platform::getFontFilename(const std::string &sTextFace) {
  std::string fontFileReturn;

#if defined(__linux__)

  FcConfig *config = FcInitLoadConfigAndFonts();

  // configure the search pattern,
  // assume "name" is a std::string with the desired font name in it
  FcPattern *pat = FcNameParse((const FcChar8 *)(sTextFace.c_str()));
  FcConfigSubstitute(config, pat, FcMatchPattern);
  FcDefaultSubstitute(pat);

  // find the font
  FcResult ret;
  FcPattern *font = FcFontMatch(config, pat, &ret);
  if (font) {
    FcChar8 *file = NULL;
    if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
      // save the file to another std::string
      fontFileReturn = (char *)file;
    }
    FcPatternDestroy(font);
  }

  FcPatternDestroy(pat);

#elif defined(_WIN64)

  wstring subKey = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";

  HKEY regKey = 0;
  LONG ret;
  wstring wsName;
  DWORD dwNameSize;
  vector<BYTE> vValue;
  DWORD dwValueSize;
  DWORD dwType = 0;
  DWORD index = 0;
  wstring wsSearch;

  // convert input string to a multibyte wstring.
  int wsSize =
      MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, sTextFace.data(),
                          sTextFace.size(), wsSearch.data(), 0);
  wsSearch.resize(wsSize);
  MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, sTextFace.data(),
                      sTextFace.size(), wsSearch.data(), wsSize);

  // open the registry key
  ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_READ, &regKey);
  if (ret != ERROR_SUCCESS) {
    string errorCode;
    errorCode = "Could not open windows registry (" + to_string(ret) + ")";
    throw std::runtime_error(errorCode);
  }

  // get the number of entries
  DWORD valueCount;
  DWORD maxValueNameLen;
  DWORD maxValueLen;

  // get the numer of items and the maximum lengths.
  ret = RegQueryInfoKey(regKey, nullptr, nullptr, nullptr, nullptr, nullptr,
                        nullptr, &valueCount, &maxValueNameLen, &maxValueLen,
                        nullptr, nullptr);
  if (ret != ERROR_SUCCESS) {
    string errorCode;
    errorCode = "Could not open windows registry (" + to_string(ret) + ")";
    throw std::runtime_error(errorCode);
  }

  // resize for space and clear
  maxValueLen++;
  maxValueNameLen++;
  wsName.resize(maxValueNameLen);
  vValue.resize(maxValueLen);
  bool bFound = false;

  // look for a match
  for (DWORD index = 0; index < valueCount; index++) {

    // get registry value
    dwNameSize = maxValueNameLen;
    dwValueSize = maxValueLen;
    ret = RegEnumValueW(regKey, index, wsName.data(), &dwNameSize, NULL,
                        &dwType, vValue.data(), &dwValueSize);
    if (ret != ERROR_SUCCESS) {
      string errorCode;
      errorCode = "Could not open windows registry (" + to_string(ret) + ")";
      throw std::runtime_error(errorCode);
    }

    // match name or file names
    if (_wcsnicmp(wsName.c_str(), wsSearch.c_str(), wsSearch.size()) == 0) {
      bFound = true;
      break;
    }
  }

  // close the registry key
  RegCloseKey(regKey);

  /**
    Some font filenames within the registry have the path built in
    while most windows fonts do not. This provides the full path.
    For simplicity, check the second and third characters of the
    file name to determine if it has a drive specifier.
  */
  LPWSTR lpwData = reinterpret_cast<LPWSTR>(vValue.data());
  wstring wsfontFileReturn;

  // if (!bFound)
  // wcscpy_s(lpwData, L"arial.ttf");

  if (_wcsnicmp(lpwData + 1, L":\\", 2) == 0) {
    wsfontFileReturn = lpwData;

  } else {
    wsfontFileReturn.resize(MAX_PATH);
    int lLen = GetSystemWindowsDirectoryW(wsfontFileReturn.data(), MAX_PATH);
    wsfontFileReturn.resize(lLen);
    wsfontFileReturn.append(L"\\Fonts\\");
    wsfontFileReturn.append(lpwData);
  }

  // convert the widestring to an utf8 or default character representation
  fontFileReturn.resize(MAX_PATH);
  DWORD dw =
      WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                          wsfontFileReturn.data(), wsfontFileReturn.size(),
                          fontFileReturn.data(), MAX_PATH, nullptr, nullptr);
  fontFileReturn.resize(dw);
#endif

  return fontFileReturn;
}
#endif // defined

/**
\internal
\brief The drawText function provides textual character rendering.

\param std::string
Optimized Blend
https://www.codeguru.com/cpp/cpp/algorithms/general/article.php/c15989/Tip-An-Optimized-Formula-for-Alpha-Blending-Pixels.htm

*/
void uxdevice::platform::activateTextFace(void) {
#if defined(USE_FREETYPE)
  // store a cache record for loaded fonts.
  m_faceID = getFaceID(*m_textFace);

  // having this as a local variable
  m_scaler.face_id = m_faceID;
  m_scaler.pixel = 0;
  m_scaler.height = (*m_pointSize + fontScale) * 64;
  m_scaler.width = (*m_pointSize + fontScale) * 64;

  m_scaler.x_res = 96;
  m_scaler.y_res = 96;

  // get the face
  m_error = FTC_Manager_LookupSize(m_cacheManager, &m_scaler, &m_sizeFace);

  if (m_error)
    throw std::runtime_error("Could not retrieve font face.");

  m_error = FT_Activate_Size(m_sizeFace);
  if (m_error)
    throw std::runtime_error("Could FT_Activate_Size for font.");
  m_faceHeight = m_sizeFace->face->size->metrics.height >> 6;

#endif // defined

}

/**
\internal
\brief The drawText function provides textual character rendering.

\param std::string
Optimized Blend
https://www.codeguru.com/cpp/cpp/algorithms/general/article.php/c15989/Tip-An-Optimized-Formula-for-Alpha-Blending-Pixels.htm

*/
#if defined(USE_FREETYPE)
void uxdevice::platform::renderText(const std::size_t &beginIndex,
                                    const std::size_t &endIndex) {

  // set the text pen rendering position
  m_xpos = m_targetArea->x1;
  m_ypos = m_targetArea->y1;

  m_bProcessedOnce = false;

  // iterate characters in string
  for (std::size_t idx = beginIndex; idx < endIndex; idx++) {

    // exit when rectangle has been filled
    if (m_ypos > m_targetArea->y2)
      break;

    renderChar((*m_stringData)[idx]);
  }
}

/**
\internal
\brief The function provides the rendering of a individual character
\details
The routine provides the individual rendering of a character.

Special
characters such as escape characters are filtered.


\param const char c is the individual character

*/
int uxdevice::platform::renderChar(const char c) {
  FT_Error error;
  unsigned int color = 0x00; // computed color
  int x, y;
  const int CtabStap = 50;

  // handle special characters
  // new line
  switch (c) {
  case '\n':
    m_xpos = m_targetArea->x1;
    m_ypos += m_faceHeight;
    return 0;
    break;
  case '\t':
    m_xpos += CtabStap;
    return CtabStap;
    break;
  }

  // get the index of the glyph
  m_glyph_index = FTC_CMapCache_Lookup(m_cmapCache, m_faceID, 0, c);
  FT_Face face = m_sizeFace->face;

  // the kerning of a font depends on the previous character
  // some proportional fonts provide tighter spacing which improves
  // rendering characteristics
  if (m_bProcessedOnce && FT_HAS_KERNING(face)) {
    FT_Vector akerning;
    error = FT_Get_Kerning(face, m_previous_index, m_glyph_index,
                           FT_KERNING_DEFAULT, &akerning);
    if (!error) {
      m_xpos += akerning.x >> 6;
    }
  }

  // get the height of the font
  int baseline = face->size->metrics.ascender >> 6;

  /**
  \brief use greyscale or color lcd filtering
\details
      There are two distinct types of bimap structures that are in use, grey
      scale or lcd filtered. The buffer format is unique for each, the grey
  lite one being a value indicating grey luminence while the lcd filter is a
  rgb one. These values provide the same functionality for the looping and
      drawing routine. You will notice that within each block of code, after
      getting the image, these values are set.
  */
  int storageSize, pitch, top, left, height, width, xadvance;
  unsigned char *buffer;

#ifdef USE_FREETYPE_GREYSCALE_ANTIALIAS
  // get the image
  FTC_SBit bitmap;
  error = FTC_SBitCache_LookupScaler(m_bitCache, &m_scaler, FT_LOAD_RENDER,
                                     m_glyph_index, &bitmap, nullptr);

  if (error)
    return 0;

  // set the rendering values used for grey
  storageSize = 1;
  pitch = bitmap->pitch;
  top = bitmap->top;
  left = bitmap->left;
  height = bitmap->height;
  width = bitmap->width;
  buffer = bitmap->buffer;
  xadvance = bitmap->xadvance;

#elif defined (USE_FREETYPE_LCD_FILTER)
  FT_Glyph aglyph;
  FT_BitmapGlyph bitmap;

  // get the image, however this is just the outline
  error = FTC_ImageCache_LookupScaler(m_imageCache, &m_scaler, FT_LOAD_DEFAULT,
                                      m_glyph_index, &aglyph, nullptr);
  if (error)
    return 0;

  xadvance = (aglyph->advance.x + 0x8000) >> 16;

  // this converts the outline image to a rgb bitmap
  error = FT_Glyph_To_Bitmap(&aglyph, FT_RENDER_MODE_LCD, 0, 0);
  bitmap = reinterpret_cast<FT_BitmapGlyph>(aglyph);

  // set the rendering values used for for grey
  storageSize = 3;
  pitch = bitmap->bitmap.pitch;
  top = bitmap->top;
  left = bitmap->left;
  height = bitmap->bitmap.rows;
  width = bitmap->bitmap.width;
  buffer = bitmap->bitmap.buffer;

#endif

  x = m_xpos;
  y = m_ypos + baseline - top;

  // calculate the maximum bounds
  int xmax = x + width / storageSize + left;
  int ymax = m_ypos + (baseline - top) + height;

  // if the maximum bounds are greater than the clipping region, adjust.
  if (xmax > m_targetArea->x2)
    xmax = m_targetArea->x2;
  if (ymax > m_targetArea->y2)
    ymax = m_targetArea->y2;

  // loop through the pixels
  for (int i = x + left, p = 0; i < xmax; i++, p += storageSize) {
    for (int j = y, q = 0; j < ymax; j++, q++) {
      int bufferPosition = q * pitch + p;

      /* only plot active pixels, the term
       luminance is used because it is not actually a color
       but a brightness of the pixel. Zero being off for the
       glyph bitmap. */
#ifdef USE_FREETYPE_GREYSCALE_ANTIALIAS
      if (buffer[bufferPosition]) {
#elif defined USE_FREETYPE_LCD_FILTER
      if (buffer[bufferPosition] || buffer[bufferPosition + 1] ||
          buffer[bufferPosition + 2]) {

#endif

#if defined(USE_IMAGE_MAGICK)
        Magick::Color destinationC = getPixel(i, j);

        unsigned char destinationR = destinationC.quantumRed() / QuantumRange * 255;
        unsigned char destinationG = destinationC.quantumGreen() / QuantumRange * 255;
        unsigned char destinationB = destinationC.quantumBlue() / QuantumRange * 255;

#else
        unsigned int destinationC = getPixel(i, j);
        unsigned char destinationR = destinationC >> 16;
        unsigned char destinationG = destinationC >> 8;
        unsigned char destinationB = destinationC;
#endif // defined


#if defined(USE_FREETYPE_GREYSCALE_ANTIALIAS)
        // luminescence is expressed in gray scale using one byte
        unsigned char freetypeColor = buffer[bufferPosition];
        unsigned char freetypeR = freetypeColor;
        unsigned char freetypeG = freetypeColor;
        unsigned char freetypeB = freetypeColor;

#elif defined(USE_FREETYPE_LCD_FILTER)
        // luminescence is expressed within the LCD format as three bytes.
        unsigned char freetypeR = buffer[bufferPosition];
        unsigned char freetypeG = buffer[bufferPosition + 1];
        unsigned char freetypeB = buffer[bufferPosition + 2];
#endif

        unsigned char targetR =
            ((m_textColorR * freetypeR) + (destinationR * (255 - freetypeR))) >>
            8;
        unsigned char targetG =
            ((m_textColorG * freetypeG) + (destinationG * (255 - freetypeG))) >>
            8;
        unsigned char targetB =
            ((m_textColorB * freetypeB) + (destinationB * (255 - freetypeB))) >>
            8;

        color = ((targetR) << 16) | ((targetG) << 8) | (targetB);
#if defined(USE_IMAGE_MAGICK)
        Magick::Color mgColor(targetR/255.0*QuantumRange,targetG/255.0*QuantumRange,targetB/255.0*QuantumRange);
        putPixel(i, j, mgColor);
#else
        // place the computed color into pixel buffer
        putPixel(i, j, color);
#endif
      }
    }
  }

#ifdef USE_FREETYPE_LCD_FILTER
  // delete the bitmap data
  FT_Done_Glyph((FT_Glyph)bitmap);

#endif
  m_previous_index = m_glyph_index;
  m_bProcessedOnce = true;
  m_xpos += xadvance;

  return xadvance;
}
#endif

void uxdevice::platform::renderImage(const rectangle &src) {
  int clampedWidth = 0;
  int clampedHeight = 0;

  int targetWidth = m_targetArea->x2 - m_targetArea->x1;
  int targetHeight = m_targetArea->y2 - m_targetArea->y1;

#if defined(USE_STB_IMAGE)
  int destX, destY;
  if (*m_width > targetWidth) {
    clampedWidth = targetWidth;
  } else {
    clampedWidth = *m_width;
  }
  if (*m_height > targetHeight) {
    clampedHeight = targetHeight;
  } else {
    clampedHeight = *m_height;
  }
  for (int j = 0; j < clampedHeight; j++) {
    for (int i = 0; i < clampedWidth; i++) {
      destX = m_targetArea->x1 + i;
      destY = m_targetArea->y1 + j;

      size_t bufferPos = i * 4 + j * 4 * (*m_width);
      unsigned int *p =
          reinterpret_cast<unsigned int *>(&(*m_imageData)[bufferPos]);
      putPixel(destX, destY, *p);

    }
  }

#elif defined(USE_IMAGE_MAGICK)
  if (m_image->columns() > targetWidth) {
    clampedWidth = targetWidth;
  } else {
    clampedWidth = m_image->columns();
  }
  if (m_image->rows() > targetHeight) {
    clampedHeight = targetHeight;
  } else {
    clampedHeight = m_image->rows();
  }

  m_image->crop(Magick::Geometry(clampedWidth,clampedHeight));
  m_offscreenImage.composite(*m_image,m_targetArea->x1,
                              m_targetArea->y1,
                              Magick::CompositeOperator::CopyCompositeOp);
#endif


}


#if defined(USE_FREETYPE)
/**
\brief The routine returns that face ID for the cached font. This is a
pointer to the record within the vector.
*/
FTC_FaceID uxdevice::platform::getFaceID(string sTextFace) {
  FTC_FaceID faceID = nullptr;

  auto it = m_faceCache.find(sTextFace);
  if (it != m_faceCache.end()) {
    faceID = static_cast<FTC_FaceID>(&it->second);
  } else {
    string sFullFontPath = getFontFilename(sTextFace);
    faceCacheStruct faceCacheRecord{sFullFontPath,
                                    static_cast<int>(m_faceCache.size())};
    pair<faceCacheIterator, bool> result =
        m_faceCache.insert({sTextFace, faceCacheRecord});
    // A potential bug may exist when items are added while the faceID is
    // waiting to be used.
    if (result.second)
      faceID = static_cast<FTC_FaceID>(&(result.first->second));
  }
  return faceID;
}
#endif // defined

/**
\internal
\brief The function returns the width of the string according to the font
size.
*/
int uxdevice::platform::measureTextWidth(const std::string &sTextFace,
                                         const int pointSize,
                                         const std::string &s) {

#if defined(USE_FREETYPE)
  bool bProcessedOnce = false;
  FT_Error error;
  FTC_ScalerRec scaler;
  FT_Size sizeFace;
  FT_UInt glyph_index = 0;
  FT_UInt previous_index = 0;
  int returnedWidth = 0;

  // store a cache record for loaded fonts.
  FTC_FaceID faceID = getFaceID(sTextFace);

  // having this as a local variable
  scaler.face_id = faceID;
  scaler.pixel = 0;
  scaler.height = (pointSize + fontScale) * 64;
  scaler.width = (pointSize + fontScale) * 64;

  scaler.x_res = 96;
  scaler.y_res = 96;

  // get the face
  error = FTC_Manager_LookupSize(m_cacheManager, &scaler, &sizeFace);

  if (error)
    throw std::runtime_error("Could not retrieve font face.");

  error = FT_Activate_Size(sizeFace);
  if (error)
    throw std::runtime_error("Could FT_Activate_Size for font.");

  FT_Face face = sizeFace->face;

  // iterate characters in string
  for (auto &c : s) {

    // handle special characters
    if (c == '\t') {
      returnedWidth += 50;
      continue;
    }

    // get the index of the glyph
    glyph_index = FTC_CMapCache_Lookup(m_cmapCache, faceID, 0, c);

    // the kerning of a font depends on the previous character
    // some proportional fonts provide tighter spacing which improves
    // rendering characteristics
    if (bProcessedOnce && FT_HAS_KERNING(face)) {
      FT_Vector akerning;
      error = FT_Get_Kerning(face, previous_index, glyph_index,
                             FT_KERNING_DEFAULT, &akerning);
      if (!error) {
        returnedWidth += akerning.x >> 6;
      }
    }

    int xadvance;

#ifdef USE_FREETYPE_GREYSCALE_ANTIALIAS
    // get the image
    FTC_SBit bitmap;
    error = FTC_SBitCache_LookupScaler(m_bitCache, &scaler, FT_LOAD_RENDER,
                                       glyph_index, &bitmap, nullptr);

    if (error)
      continue;

    xadvance = bitmap->xadvance;

#elif defined USE_FREETYPE_LCD_FILTER
    FT_Glyph aglyph;
    FT_BitmapGlyph bitmap;

    // get the image, however this is just the outline
    error = FTC_ImageCache_LookupScaler(m_imageCache, &scaler, FT_LOAD_DEFAULT,
                                        glyph_index, &aglyph, nullptr);
    if (error)
      continue;

    xadvance = (aglyph->advance.x + 0x8000) >> 16;

#endif

    // move after render
    returnedWidth += xadvance;
    bProcessedOnce = true;
    previous_index = glyph_index;
  }
  return returnedWidth;
#endif // defined

}

/**
\internal
\brief the function measures the height of the textFace
\param const std::string &sTextFace the face name
\param const int pointSize the size in point of the font

*/
int uxdevice::platform::measureFaceHeight(const std::string &sTextFace,
                                          const int pointSize) {
#if defined(USE_FREETYPE)
  FT_Error error;
  FTC_ScalerRec scaler;
  FT_Size sizeFace;

  // store a cache record for loaded fonts.
  FTC_FaceID faceID = getFaceID(sTextFace);

  // having this as a local variable
  scaler.face_id = faceID;
  scaler.pixel = 0;
  scaler.height = (pointSize + fontScale) * 64;
  scaler.width = (pointSize + fontScale) * 64;

  scaler.x_res = 96;
  scaler.y_res = 96;

  // get the face
  error = FTC_Manager_LookupSize(m_cacheManager, &scaler, &sizeFace);

  if (error)
    throw std::runtime_error("Could not retrieve font face.");

  error = FT_Activate_Size(sizeFace);
  if (error)
    throw std::runtime_error("Could FT_Activate_Size for font.");

  FT_Face face = sizeFace->face;

  // get the height of the font
  int faceHeight = face->size->metrics.height >> 6;
  return static_cast<int>(faceHeight);
#endif // defined

}

/**
  \internal
  \brief the function draws the cursor.
  */
void uxdevice::platform::drawCaret(const int x, const int y, const int h) {
  for (int j = y; j < y + h; j++)
    putPixel(x, j, 0x00);
}

/**
\internal
\brief the function clears the dirty rectangles of the off screen buffer.
*/
void uxdevice::platform::clear(void) {
#if defined(USE_DIRECT_SCREEN_OUTPUT) && !defined(USE_IMAGE_MAGICK)
  fill(m_offscreenBuffer.begin(), m_offscreenBuffer.end(), 0xFF);

#elif defined(USE_IMAGE_MAGICK)
  m_offscreenImage.strokeColor("white"); // Outline color
  m_offscreenImage.fillColor("white"); // Fill color
  m_offscreenImage.strokeWidth(0);
  m_offscreenImage.draw( Magick::DrawableRectangle(0,0, m_offscreenImage.columns(),m_offscreenImage.rows()));
#endif // defined

  m_xpos = 0;
  m_ypos = 0;
}

/**
\brief The function places a color into the offscreen pixel buffer.
    Coordinate start at 0,0, upper left.
\param x - the left point of the pixel
\param y - the top point of the pixel
\param unsigned int color - the bgra color value

*/
#if defined(USE_DIRECT_SCREEN_OUTPUT) && !defined(USE_IMAGE_MAGICK)
void uxdevice::platform::putPixel(const int x, const int y,
                                  const unsigned int color) {
  if (x < 0 || y < 0)
    return;

  // clip coordinates
  if (x >= _w || y >= _h)
    return;

  // calculate offset
  unsigned int offset = x * 4 + y * 4 * _w;

  // put rgba color
  unsigned int *p =
      reinterpret_cast<unsigned int *>(&m_offscreenBuffer[offset]);
  *p = color;
}

#elif defined(USE_IMAGE_MAGICK)
void uxdevice::platform::putPixel(const int x, const int y,
                                  const Magick::Color color) {

  if (x < 0 || y < 0)
    return;

  // clip coordinates
  if (x >= _w || y >= _h)
    return;

  Magick::Quantum *p = &m_offscreenBuffer[x*4+y*_w*4];
  *p=color.quantumRed();
  p++;
  *p=color.quantumGreen();
  p++;
  *p=color.quantumBlue();
  p++;

}
#endif // defined


/**
\brief The function returns the color at the pixel space. Coordinate start
at 0,0, upper left. \param x - the left point of the pixel \param y - the
top point of the pixel \param unsigned int color - the bgra color value

*/
#if defined(USE_DIRECT_SCREEN_OUTPUT) && !defined(USE_IMAGE_MAGICK)
unsigned int uxdevice::platform::getPixel(const int x, const int y) {
  // clip coordinates
  if (x < 0 || y < 0)
    return 0;

  if (x >= _w || y >= _h)
    return 0;

  // calculate offset
  unsigned int offset = x * 4 + y * 4 * _w;

  // put rgba color
  unsigned int *p =
      reinterpret_cast<unsigned int *>(&m_offscreenBuffer[offset]);
  return *p;

}

#elif defined(USE_IMAGE_MAGICK)
Magick::Color uxdevice::platform::getPixel(const int x, const int y) {
  if (x < 0 || y < 0)
    return Magick::Color("black");

  // clip coordinates
  if (x >= _w || y >= _h)
    return Magick::Color("black");

  Magick::Quantum *p = &m_offscreenBuffer[x*4+y*m_offscreenImage.columns()*4];
  return Magick::Color(*p,*(p+1),*(p+2));
}
#endif // defined

/**
\brief The function provides the reallocation of the offscreen buffer

*/
void uxdevice::platform::resize(const int w, const int h) {

  _w = w;
  _h = h;

#if defined(__linux__)

  // free old one if it exists
  if (m_pix) {
    xcb_shm_detach(m_connection, m_info.shmseg);
    shmdt(m_info.shmaddr);
    xcb_free_pixmap(m_connection, m_pix);
  }

  // Shared memory test.
  // https://stackoverflow.com/questions/27745131/how-to-use-shm-pixmap-with-xcb?noredirect=1&lq=1
  xcb_shm_query_version_reply_t *reply;

  reply = xcb_shm_query_version_reply(
      m_connection, xcb_shm_query_version(m_connection), NULL);

  if (!reply || !reply->shared_pixmaps) {
    cout << "Could not get a shared memory image." << endl;
    exit(0);
  }

  size_t _bufferSize = _w * _h * 4;

  m_info.shmid = shmget(IPC_PRIVATE, _bufferSize, IPC_CREAT | 0600);
  m_info.shmaddr = (uint8_t *)shmat(m_info.shmid, 0, 0);

  m_info.shmseg = xcb_generate_id(m_connection);
  xcb_shm_attach(m_connection, m_info.shmseg, m_info.shmid, 0);
  shmctl(m_info.shmid, IPC_RMID, 0);

  m_screenMemoryBuffer = static_cast<uint8_t *>(m_info.shmaddr);

  m_pix = xcb_generate_id(m_connection);
  xcb_shm_create_pixmap(m_connection, m_pix, m_window, _w, _h,
                        m_screen->root_depth, m_info.shmseg, 0);

  m_offscreenBuffer.resize(_bufferSize);

  // clear to white
  clear();

#elif defined(_WIN64)
  // get the size ofthe window
  RECT rc;
  GetClientRect(m_hwnd, &rc);

  // resize the pixel memory
  _w = rc.right - rc.left;
  _h = rc.bottom - rc.top;

  int _bufferSize = _w * _h * 4;

  m_offscreenBuffer.resize(_bufferSize);

  // clear to white
  clear();

  // free existing resources
  if (m_pRenderTarget) {
    m_pRenderTarget->Resize(D2D1::SizeU(_w, _h));

  } else {

    // Create a Direct2D render target
    HRESULT hr = m_pD2DFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(m_hwnd, D2D1::SizeU(_w, _h)),
        &m_pRenderTarget);
    if (FAILED(hr))
      return;
  }
#endif
}

/**
\brief The function copies the pixel buffer to the screen

*/
void uxdevice::platform::flip() {
#if defined(__linux__)

  // copy offscreen data to the shared memory video buffer
  memcpy(m_screenMemoryBuffer, m_offscreenBuffer.data(),
         m_offscreenBuffer.size());

  // blit the shared memory buffer
  xcb_copy_area(m_connection, m_pix, m_window, m_graphics, 0, 0, 0, 0, _w, _h);

  xcb_flush(m_connection);

#elif defined(_WIN64)
  if (!m_pRenderTarget)
    return;

  m_pRenderTarget->BeginDraw();

  // create offscreen bitmap for pixel rendering
  D2D1_PIXEL_FORMAT desc2D = D2D1::PixelFormat();
  desc2D.format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc2D.alphaMode = D2D1_ALPHA_MODE_IGNORE;

  D2D1_BITMAP_PROPERTIES bmpProperties = D2D1::BitmapProperties();
  m_pRenderTarget->GetDpi(&bmpProperties.dpiX, &bmpProperties.dpiY);
  bmpProperties.pixelFormat = desc2D;

  RECT rc;
  GetClientRect(m_hwnd, &rc);

  D2D1_SIZE_U size = D2D1::SizeU(_w, _h);
  HRESULT hr = m_pRenderTarget->CreateBitmap(
      size, m_offscreenBuffer.data(), _w * 4, &bmpProperties, &m_pBitmap);

  // render bitmap to screen
  D2D1_RECT_F rectf;
  rectf.left = 0;
  rectf.top = 0;
  rectf.bottom = _h;
  rectf.right = _w;

  m_pRenderTarget->DrawBitmap(m_pBitmap, rectf, 1.0f,
                              D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);

  m_pRenderTarget->EndDraw();
  m_pBitmap->Release();

#endif
}

uxdevice::imageData::imageData(std::shared_ptr<int> _width,
                               std::shared_ptr<int> _height,
                               std::shared_ptr<std::vector<u_int8_t>> _data) {

#ifdef USE_STB_IMAGE
  width = _width;
  height = _height;
  data = _data;
#endif // USE_STB_IMAGE
}

uxdevice::imageData::imageData(std::shared_ptr<std::string> _fileName) {
  fileName = _fileName;

#if defined(USE_STB_IMAGE)
  int w, h, n;
  NSVGimage *shapes = NULL;
  NSVGrasterizer *rast = NULL;
  unsigned char *localData = NULL;
  unsigned *dp;
  size_t i, len;

  if ((localData = stbi_load(_fileName->data(), &w, &h, &n, 4)))
    ;
  else if ((shapes = nsvgParseFromFile(_fileName->data(), "px", 96.0f))) {
    w = (int)shapes->width;
    h = (int)shapes->height;
    rast = nsvgCreateRasterizer();
    localData = reinterpret_cast<unsigned char *>(malloc(w * h * 4));
    nsvgRasterize(rast, shapes, 0, 0, 1, localData, w, h, w * 4);
  } else {
    string info = "Cannot load the file: ";
    info += *_fileName;
    throw std::invalid_argument(info);
  }
  data = make_shared<vector<unsigned char>>();
  data->reserve(w * h * 4);
  unsigned int *p = reinterpret_cast<unsigned int *>(data->data());
  for (i = 0, len = w * h, dp = (unsigned int *)localData; i < len; i++) {
    *p = dp[i] & 0xff00ff00 | ((dp[i] >> 16) & 0xFF) |
         ((dp[i] << 16) & 0xFF0000);
    p++;
  }
  width = make_shared<int>(w);
  height = make_shared<int>(h);

  free(localData);

#elif defined(USE_IMAGE_MAGICK)
  data = make_shared<Magick::Image>();
  data->type(Magick::TrueColorType);
  data->backgroundColor("None");
  data->read(*_fileName);
  Magick::Color bg_color = data->pixelColor(0,0);
  data->transparent(bg_color);
  //data-> matte(true);
#endif // USE_IMAGE_MAGICK
}
