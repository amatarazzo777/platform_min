
#include "uxdevice.hpp"

using namespace std;
using namespace uxdevice;

void test0(platform &vm);

void testStart(string_view sFunc) {
#if defined(CONSOLE)
  cout << sFunc << endl;
#elif defined(_WIN64)

#endif
}

eventHandler eventDispatch(const event &evt);

/****************************************************************************************************
***************************************************************************************************/
#if defined(__linux__)
int main(int argc, char **argv) {
  // handle command line here...
#elif defined(_WIN64)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /* hPrevInstance */,
                   LPSTR lpCmdLine, int /* nCmdShow */) {
  // command line
#endif

  // create the main window area. This may this is called a Viewer object.
  // The main browsing window. It is an element as well.
  auto vis = platform(eventDispatch);
  vis.openWindow("test app", 800, 600);

  // for the display list, only pointers are used.
  // this enables the changing of data without any copy.
  // all data from the uxdevice is external from it.
  // The shared pointer is used.
  // typically an api is used to fill these structures.
  auto coordinates = make_shared<rectangle>(10, 10, 300, 300);
  auto textInfo = make_shared<string>("client data");
  auto color = make_shared<unsigned int>(0x00);
  auto ptSize = make_shared<int>(10);
  auto aln = make_shared<char>('l');
  auto tf = make_shared<string>("arial");
  auto idx1 = make_shared<size_t>(0);
  auto idx2 = make_shared<size_t>(textInfo->size());

  vis.data().push_back(stringData{textInfo});
  vis.data().push_back(textFace{tf, ptSize});
  vis.data().push_back(textColor{color});
  vis.data().push_back(textAlignment{aln});
  vis.data().push_back(targetArea{coordinates});
  vis.data().push_back(drawText{idx1, idx2});

  stringstream ss;
  for(int i=0;i<100;i++) {
    ss << "Info " << i << " 0876543&*^%$##  5555555555555555hhh]\tttttthhhhhhhhhhhjjjjjjjjjjjjjjjjjjjj\n";
  }

  auto coordinates2 = make_shared<rectangle>(30, 30, 600, 600);
  auto color2 = make_shared<unsigned int>(0x0000ff);
  auto textInfo2 = make_shared<string>(ss.str());
  auto idx1b = make_shared<size_t>(0);
  auto idx2b = make_shared<size_t>(textInfo2->size());

  vis.data().push_back(stringData{textInfo2});
  vis.data().push_back(textColor{color2});
  vis.data().push_back(targetArea{coordinates2});
  vis.data().push_back(drawText{idx1b, idx2b});


  auto coordinates3 = make_shared<rectangle>(200, 200, 500, 500);
  //auto imageFileName = make_shared<string>("/home/anthony/source/nanosvg/example/drawing.svg");
  auto imageFileName = make_shared<string>("/home/anthony/source/nanosvg/example/screenshot-2.png");
//  auto imageFileName = make_shared<string>("plasma:fractal");
  vis.data().push_back(targetArea{coordinates3});
  vis.data().push_back(imageData{imageFileName});
  vis.data().push_back(drawImage{});


  auto coordinates4 = make_shared<rectangle>(400, 200, 900, 500);
  //auto imageFileName = make_shared<string>("/home/anthony/source/nanosvg/example/drawing.svg");
  auto imageFileName2 = make_shared<string>("/home/anthony/source/nanosvg/example/draw.png");
  vis.data().push_back(targetArea{coordinates4});
  vis.data().push_back(imageData{imageFileName2});
  vis.data().push_back(drawImage{});

  vis.dirty(0);
  int width = vis.pixelWidth(0);
  int height = vis.pixelHeight(0);
  vis.processEvents();

  test0(vis);
}

eventHandler eventDispatch(const event &evt) {}

/************************************************************************
************************************************************************/
void test0(platform &vm) {}
