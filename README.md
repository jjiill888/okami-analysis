# Ōkami Visual Style Demo – From Analysis to Code

This repository contains a small OpenGL demo that recreates key parts of Ōkami’s visual style
(cel shading, sumi-e outlines, ukiyo-e space, and paper texture) as a companion piece to my
visual analysis essay.

The goal is not only to make something “pretty,” but to show **how art direction becomes code** –
how Amaterasu’s sumi-e body, flat ukiyo-e space, and the Celestial Brush can be expressed as
concrete rendering techniques.

---

## 1. Environment Requirements

To build and run this demo, you need:

- A **desktop OS with OpenGL 3.3+ support**
  - Tested on: recent Linux (Debian/Ubuntu family)
- A C++ compiler with C++11 support  
  - e.g. `g++` (GCC)
- Development libraries:
  - **OpenGL** (system driver)
  - **GLEW** – OpenGL extension loading
  - **GLFW** – window & input
  - **GLM** – math library for matrices and vectors

### Recommended packages (Debian / Ubuntu)

```
sudo apt update
sudo apt install g++ \
    libglew-dev \
    libglfw3-dev \
    libglm-dev \
    mesa-utils
```

## 2. Build Instructions

In the project directory, compile with: 
```bash
g++ okami_demo.cpp -o okami_demo \
    -std=c++11 -lGL -lGLEW -lglfw -lm
```

### 3. Running the Demo

```
./okami_demo
```
