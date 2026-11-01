# Lesset

Calculator with...

Order of operations! Constants! Variables! Sliders! 100 predefined functions! Custom Functions! Custom Operators! Table calculations! Graphing! Far too many decimal places of precision! Enough operators to make your head spin! Styling! Military grade scripting! Jank!

<img src="/Assets/graphNew.png" height="75%" width="75%">

To compile:

- Install Vulkan headers, nativefiledialog-extended

- Copy the contents of ImStyle/src into the source directory of Lesset (https://github.com/csprite/ImStyle) 

- Install the Boost C++ library https://www.boost.org/

- Get a copy of NotoSansMathRegular

- Get a copy of ImGui and ImPlot and put their folders (named "imgui" and "implot") into the parent directory of where you copied this repo

- Check https://github.com/ocornut/imgui/blob/master/docs/FONTS.md#loading-font-data-embedded-in-source-code 

- Use https://github.com/ocornut/imgui/blob/master/misc/fonts/binary_to_compressed_c.cpp to turn the font into a C file.

- Rename that file to NotoSansMathRegular.hpp and make sure the names of the variables therein are "NotoSansMathRegular_compressed_size" and "NotoSansMathRegular_compressed_data"

- Copy implot.cpp, implot_demo.cpp, implot_items.cpp and imgui_stdlib.cpp into the folder with main.cpp

- Invoke cmake to build

- Pray it works lol

This readme probably still sucks.

No coding agents or LLMs or similar were used for this. In case this line disappears, I have succumbed to the dark side.
