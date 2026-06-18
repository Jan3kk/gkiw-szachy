# Szachy 3D — odtwarzacz partii (OpenGL)

Trójwymiarowy odtwarzacz partii szachowej. Przebieg partii wczytywany jest z pliku,
a położeniem obserwatora steruje użytkownik (swobodny lot kamerą). Projekt na przedmiot
„Grafika komputerowa i wizualizacja".

Biblioteki (GLEW, GLFW, GLM) oraz `lodepng` znajdują się w repozytorium — **nie trzeba
nic instalować poza kompilatorem**.

## Wymagania

- Windows
- **Visual Studio 2022 (lub nowszy)** z obciążeniem **„Desktop development with C++"**
  (zawiera kompilator MSVC, MSBuild i Windows SDK).
- Opcjonalnie: **Visual Studio Code** z rozszerzeniem **C/C++** (`ms-vscode.cpptools`).

Projekt nie jest przypięty do konkretnej wersji toolsetu (`$(DefaultPlatformToolset)`),
więc kompiluje się tym, co jest zainstalowane (VS 2019/2022/nowszy).

## Uruchomienie w Visual Studio Code

1. Otwórz folder projektu w VS Code.
2. Build: **Ctrl+Shift+B** (zadanie „build (Debug x64)" — samo znajduje MSBuild przez `vswhere`).
3. Uruchomienie / debugowanie: **F5** (najpierw zbuduje, potem uruchomi).

## Uruchomienie w Visual Studio

1. Otwórz `gkiw_nst_08_win.sln`.
2. Wybierz konfigurację **Debug / x64** i naciśnij **F5**.

> Program wczytuje shadery, teksturę nieba i plik partii ścieżkami względnymi, dlatego musi
> być uruchamiany z katalogu projektu. Konfiguracje VS Code (`cwd`) i Visual Studio robią to
> automatycznie.

## Sterowanie

| Klawisz | Akcja |
|---|---|
| **W / S / A / D** | ruch kamery przód / tył / lewo / prawo |
| **Spacja / Lewy Shift** | ruch w górę / w dół |
| **Mysz** | rozglądanie (tryb FPS) |
| **ESC** | wyjście |

## Format pliku partii

Partie leżą w folderze `games/` (domyślnie wczytywana jest `games/szewczyk.txt`).
Jeden ruch na linię, własny prosty format:

```
e2 e4        # ruch / bicie (bicie wykrywane automatycznie)
e7 e8 Q      # ruch z promocją (na hetmana)
O-O          # roszada krótka
O-O-O        # roszada długa
# linia komentarza, puste linie są pomijane
```

Pola w notacji `kolumna-wiersz` (`a1`..`h8`). Biały zaczyna, kolory naprzemiennie.
Bicie, roszada, en passant i promocja są obsługiwane. Aby odtworzyć inną partię,
podmień zawartość `games/szewczyk.txt` (lub zmień wczytywaną ścieżkę w `initOpenGLProgram`).

## Struktura kodu

- `main_file.cpp` — pętla główna, kamera, rysowanie sceny, animacja odtwarzania.
- `pieces.h` — proceduralne generowanie modeli bierek (bryły obrotowe + skoczek).
- `chess.h` — typy, parser pliku partii, ustawienie początkowe.
- `*.glsl` — shadery: `v_lit`/`f_lit` (oświetlenie 2 źródłami), `v_tex`/`f_tex` (niebo).
