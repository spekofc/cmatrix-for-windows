# cmatrix-for-windows

Chuva digital da Matrix no terminal, feita em C++.

<img width="1913" height="1080" alt="{61B387F4-B98C-41C2-8DF5-E967837F2EE5}" src="https://github.com/user-attachments/assets/8af3f41c-533b-44b1-89b4-b9affc6c5206" />


- Arquivo único, sem dependências
- Windows, Linux e macOS
- Se adapta ao redimensionar o terminal

## Compilar

```
g++ -O2 -o cmatrix cmatrix.cpp
```

MSVC:

```
cl /EHsc /O2 cmatrix.cpp
```

## Uso

Execute `cmatrix` e pressione `q`, `ESC` ou `Ctrl+C` para sair.

No Windows, use o Windows Terminal ou qualquer console com suporte a ANSI/VT (Windows 10 ou superior).

## Licença

MIT
