# Image Lite
A lightweight image viewing software with fast load times, frequently used viewing functionality, and essential UI elements.

This software uses SDL2, a cross-platform development library.
SDL2 is licensed under the zlib/libpng license.



The use functionality (for inputs) has been documented as follows:
  - Shift + 1-9: Set zoom to n × 10%.
  - Shift + B: Swap values of red and green pixels of the image.
  - Shift + G: Swap values of red and blue pixels of the image.
  - Shift + R: Swap values of green and blue pixels of the image.

  - Shift + Page Up: Zoom in by 1%.
  - Shift + Page Down: Zoom out by 1%.
  - Shift + Arrow Keys: Navigation across the image (especially when zoomed in or when original size is on) by 1 px.

  - Ctrl + ← / →: Rotate image by 5° in the anti-clockwise or clockwise direction.
  - Ctrl + Shift + ← / →: Rotate image by 1° in the anti-clockwise or clockwise direction.
  - Ctrl + Alt + ← / →: Rotate image by 45° in the anti-clockwise or clockwise direction.

  - Alt + ← / →: Load the previous or the next image in the current directory with the extension .png, .jpg, .jpeg, .bmp or .ico.

  - 0: Reset zoom to 100%, view to the centre of the image, and rotation to 0%.
  - 1-9: Set zoom to n × 100%.
  - B: Change background; cycle between solid white and solid black.
  - C: Apply cycle effect. [1]
  - H: Toggle between flipping the screen horizontally.
  - I: Toggle between viewing the image in inverted colours and otherwise.
  - O: Toggle between original size and scaled size. [2]
  - T: Change window title; cycle between filename stem, filename and full file path.

  - =: Zoom in by 100%.
  - -: Zoom out by 100%.
  - Escape: Exit Image Lite.
  - F11: Toggle fullscreen on or off.
  - Page Up: Zoom in by 10%.
  - Page Down: Zoom out by 10%.
  - Arrow Keys: Navigation across the image (especially when zoomed in or when original size is on) by 10 px.

  - Mousewheel Scrolling: Pan image horizontally and move image vertically depending on input, similar to arrow keys but by 40 px.

[1] The pixel values of the image are cycled; red to green, green to blue, and blue to red.
[2] Original size shows the image with 1:1 pixel ratio at 100% zoom. By default, the image is scaled to fit the window at 100% zoom.
