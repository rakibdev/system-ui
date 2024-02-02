### Build

Using **xmake.lua**

```
xmake build
```

Or **g++**

```
g++ -std=c++2b -shared -fPIC demo.cpp -o ~/.config/system-ui/extensions/demo.so $(pkg-config --cflags --libs system-ui)
```
