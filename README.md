# 2025-1_Computacao_Grafica_GR86006
Repositório para entregas da atividade acadêmica de computação Gráfica 2025/1

# 🏰 Castle Scene 3D - OpenGL

Este projeto exibe uma cena 3D interativa com modelos `.obj`, iluminação Phong, animação de trajetórias, e seleção de objetos via clique. Desenvolvido com OpenGL (4.5), GLFW, GLAD, GLM, stb_image e TinyObjLoader.

---

## 📦 Dependências

Certifique-se de ter os seguintes componentes instalados:

- CMake (3.10 ou superior)
- OpenGL 4.5+
- [GLFW](https://www.glfw.org/)
- [GLAD](https://glad.dav1d.de/) (loader para OpenGL)
- [GLM](https://github.com/g-truc/glm)
- [stb_image](https://github.com/nothings/stb)
- [TinyObjLoader](https://github.com/tinyobjloader/tinyobjloader)

---

## ▶️ Como Compilar

### Usando CMake

```bash
mkdir build
cd build
cmake ..
make
```

---

## 🎮 Controles

| Tecla/Mouse        | Ação                          |
| ------------------ | ----------------------------- |
| `W`, `A`, `S`, `D` | Movimentar câmera             |
| Mouse              | Olhar em volta                |
| Scroll             | Zoom                          |
| Clique esquerdo    | Selecionar objeto             |
| `X`, `Y`, `Z`      | Rotacionar objeto selecionado |
| `[`, `]`           | Reduzir/Aumentar escala       |
| `C`                | Mostrar posição da câmera     |
| `SPACE`            | Pausar ou retomar animação    |
| `ESC`              | Fechar o programa             |

---

## ⚙️ Arquivo config.txt (../Cenas/)

### formato: camera <cameraStartPosition> <cameraYaw> <cameraPitch> <cameraNear> <cameraFar>
camera -8.2 1.9 14.9 -54.4 4.6 0.1 100.0

### formato: light <pos>
light 3.0 10.0 10.0

### formato: object <.obj> <pos> <rot> <escala> <trajetoria.txt|none>
object Clouds.obj 0 15 0 0 0 0 1.0 none
object Pumpkin.obj 0 0.5 9.5 0 0 0 1.0 none
object CastleRuins.obj 0 0 0 0 0 0 0.8 trajectories.txt

## 📌 Licença
Este projeto é para fins educacionais. 

"Low poly Castle" (https://www.fab.com/listings/e64d9533-c977-4ee5-83df-759ff81c36c0) by Cosmoart is licensed under Creative Commons Attribution (https://creativecommons.org/licenses/by/4.0/).

"Stylized Clouds" (https://www.fab.com/listings/fe7db3c6-506c-4def-9f3e-b621f4a39dc1) by PolyOne Studio is licensed under Creative Commons Attribution (https://creativecommons.org/licenses/by/4.0/).