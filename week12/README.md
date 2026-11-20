# 12-апта: бірнеше текстура және blending

Үш текстура жүктеледі: RGB контейнер, RGB өрнек және RGBA логотип. Міндетті бес тапсырма пернелермен көрсетіледі.

## Компиляция (Ubuntu/WSL)

```bash
g++ week12/main.cpp external/glad/src/gl.c \
  -Iexternal/glad/include -Iexternal/stb \
  -DASSET_DIR=\"week12/assets\" \
  -lglfw -lGL -ldl -o app12
./app12
```

## Басқару

- `M`: 8 режимді кезекпен ауыстыру: mix, multiply, add, clamped add, alpha decal, UV gradient, smoothstep және үш текстура.
- `Up/Down`: blend 0..1.
- `1`: texture unit қатесін қосу/өшіру.
- `2`: sampler-ге unit орнына texture ID беру қатесін қосу/өшіру.
- `B`: автоматты blend (Қ2).
- `F`: кубтың әр жағына 0.0-ден 1.0-ге дейін бөлек blend (Қ3).
- `O`: логотипті уақыт бойынша айналдыру (Қ4).
- `Tab`: тінтуірді босату/қайта ұстау; `P`: камера мен blend күйін шығару.
- `WASD`, `Space`, `Shift`, тінтуір, дөңгелек: камера; `R`: reset.

## Қорғау жауаптары

1. `glActiveTexture` болмаса, pattern 0-unit-тегі container-ді ығыстырады, 1-unit бос қалады. Сондықтан blend=0 кезінде pattern, blend=1 кезінде қара түс оқылады.
2. Sampler uniform-ға texture ID емес, unit нөмірі (`0`, `1`, `2`) беріледі. ID кездейсоқ 1 болғанда қате уақытша жасырынуы мүмкін, кейін ID өзгергенде көрінеді.
3. 0..1 сандарының көбейтіндісі бастапқы мәндерден үлкен емес, сондықтан multiply қараңғылатады және кір/даққа қолайлы. Add жарық/жалынға жарайды, бірақ қанығуы мүмкін.
4. `alpha=0` - container; `alpha=1` - logo; `0<alpha<1` - жұмсақ жиек. `logo.png` терминалда `channels: 4` болуы тиіс.
5. UV-blend terrain материалдарын биіктік/бағыт бойынша ауыстыруда қолданылады. Біркелкі емес ауысу үшін үшінші grayscale blend-mask қолданылады.

Қ1, Қ2, Қ3 және Қ4 орындалды. Кодта тек 12-аптаға дейін өткен VAO/VBO, shader, uniform, delta time, GLM матрицалары, камера, UV және texture unit тақырыптары қолданылған.
