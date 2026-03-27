# Idet
Simple terminal text editor.

# Installation
For Ai-Completions(currently unavaiable) you need to install llama.cpp.
Get llama.cpp via: https://github.com/ggml-org/llama.cpp

Install dependencies:
```bash
sudo apt install libncurses-dev
```
Replace the placeholders and run the compiler command:
```bash
g++ main.cpp src/LlamaClient.cpp -I./headers -I"<path-to-llama.cpp>/include" -L"<path-to-llama.cpp>" -lllama -lncurses -lcurl -o editor
```
### Made with Love by Laurens Emanuel Reisinger

#### Inspired by the Legendary Tedi editor
###### Tedi editor by Micheal Niederle: https://github.com/funkylang/funky_beta
