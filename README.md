# 808 KILLA – Low End Damage Unit

Audio efekt pro 808 (VST3 pro Windows a Mac, AU pro Logic). Vložíš ho na mixer insert s 808,
vybereš styl, otočíš KILL a pár maker – hotovo. Plugin nemá sampler, zpracovává jen audio, které do něj přijde.

![808 KILLA](Resources/source/design.png)

## Stažení hotového pluginu

GitHub ho sestaví sám po každé změně:

1. Záložka **Actions** → poslední běh **Build 808 KILLA** se zelenou fajfkou.
2. Dole v **Artifacts** stáhni:
   - **808-KILLA-Installer-Windows** – instalátor (.exe), nainstaluje VST3 do `C:\Program Files\Common Files\VST3`
   - nebo **808-KILLA-VST3-Windows** – samotná složka `808 KILLA.vst3` na ruční zkopírování
   - **808-KILLA-Installer-macOS** – instalátor (.pkg) pro Mac (VST3 + AU)
3. Ve FL Studiu: *Options → Manage plugins → Find more plugins*.

## Co umí (verze 0.6)

Jedna obrazovka. **Po vložení zní 808 přesně jako originál** a každý knob na ni jen přidává efekt.

- **Hlava uprostřed:** táhnutím čelisti dolů = **KILL** (zesílí všechny efekty najednou). V puse běží vlna: bílá čára = vstup, červená = výstup, světlé okraje = clipper.
- **Vpravo:** **CLIP** (hlasitější a tvrdší, clipper bez aliasingu), **DIRT** (zkreslení, typ podle presetu), **METAL** (kovová rezonance), **BUZZ** (bzučení nad subem), **LENGTH** (kratší / delší 808), **MIX**.
- **MASTER vlevo:** IN/OUT, PEAK, LUFS, CLIP, nota a tónina, PHONE check.
- **Dole:** preset `< KATEGORIE / NÁZEV >`, A/B, SAVE, OUTPUT. Kliknutím na název tabulka 96 presetů ve 12 kategoriích.

Sub pod efekty zůstává čistý, efekty reagují stejně na tichou i hlasitou 808 (AUTO LEVEL uvnitř, hlasitost se vrací zpět).

## Build na vlastním PC

Visual Studio 2022 nebo novější s *Desktop development with C++*.
Spusť `build.bat` z *Developer Command Prompt for VS*, nebo ve Visual Studiu *File → Open → Folder* a *Build All*.
Výsledek: `build\K808_artefacts\Release\VST3\808 KILLA.vst3`. Testy: `build\tests\K808Tests_artefacts\Release\K808Tests.exe`.

## Kontrola kvality (běží automaticky v GitHub Actions)

- unit testy DSP (`tests/Tests.cpp`): všechny presety na 44,1–192 kHz, žádné NaN, offline = realtime,
  bypass bez ztráty, ticho po zastavení, mono, sidechain, uložení stavu, 10 instancí, CPU
- **pluginval** strictness 10 (Windows + Mac), **auval** (Mac)

## Struktura

- `Source/DSP/` – zvukové jádro (Engine, filtry)
- `Source/Parameters.*` – parametry (ID se po vydání 1.0 nesmí měnit)
- `Source/Presets.*` – factory presety + uživatelské presety
- `Source/UI/`, `Source/PluginEditor.*` – grafika
- `installer/` – instalátory (Inno Setup, pkg) a EULA
- `tools/` – skripty, které z původního návrhu vyrobí pozadí a texturu knobů
