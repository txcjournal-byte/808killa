# 808 KILLA – Low End Damage Unit

Distortion / saturation plugin for 808s and bass (VST3 + Standalone), built with [JUCE](https://juce.com).
Works in FL Studio, Ableton Live and other DAWs that support VST3.

![808 KILLA](Resources/source/design.png)

## Jak získat hotový plugin (nejjednodušší)

GitHub ho sestaví sám po každém pushi:

1. Na GitHubu otevři záložku **Actions** → poslední běh **Build 808 KILLA (Windows VST3)**.
2. Dole v sekci **Artifacts** stáhni **808-KILLA-VST3-Windows** (zip).
3. Rozbal ho a složku **`808 KILLA.vst3`** zkopíruj do
   `C:\Program Files\Common Files\VST3`
4. Ve FL Studiu: *Options → Manage plugins → Find more plugins* (rescan) a přidej **808 KILLA**.

## Build na vlastním PC (Visual Studio 2022)

Potřebuješ Visual Studio 2022 s balíčkem **Desktop development with C++** (obsahuje i CMake).
Při prvním buildu se automaticky stáhne knihovna JUCE (je potřeba internet).

**Varianta A – dvojklik:**
Otevři *Developer Command Prompt for VS 2022*, přejdi do složky projektu a spusť `build.bat`.
Po dokončení se otevře složka s `808 KILLA.vst3`.

**Varianta B – ve Visual Studiu:**
*File → Open → Folder…* → vyber tuhle složku → nahoře zvol konfiguraci **x64-Release** →
*Build → Build All*.

Výsledek: `build\K808_artefacts\Release\VST3\808 KILLA.vst3`

## Ovládání

| Prvek | Co dělá |
|---|---|
| **PUNCH** | zvýrazní úder (transient) 808 |
| **SUB** | zesílí sub basy (~70 Hz) |
| **DISTORT** | síla zkreslení (drive) |
| **CLIP** | jak tvrdě se signál ořezává |
| **SHORT** | zkrátí dozvuk 808 (gate) |
| **BOOST** | +6 dB do zkreslení |
| **HARD CLIP** | tvrdý clip místo měkkého |
| **GRIT** | bitcrush / lo-fi špína |
| **LOW MONO** | basy pod 120 Hz do mono |
| **COOK** | extra saturační stupeň |
| **TYPE** | charakter zkreslení (SPINZ, ZAY, SHORT, BOOST, CRUNCH, RAGE, DIRTY, SUB) |
| **MODE** | celkový mód (CLEAN, CLIPPED, UNDERGROUND, DEEP, DESTROY) |
| **MIX** | poměr čistého a zpracovaného signálu |
| **OUTPUT** | výstupní hlasitost −24…+24 dB |
| **LIMITER / CEILING** | limiter na výstupu se stropem −12…0 dB |

Knoby se ovládají tažením myší nahoru/dolů, dvojklik vrátí výchozí hodnotu.
Okno pluginu jde zvětšit/zmenšit tažením za pravý dolní roh.

## Struktura projektu

- `Source/PluginProcessor.*` – zvukové zpracování
- `Source/PluginEditor.*` – grafika a ovládací prvky
- `Resources/background.png` – pozadí (vygenerované z návrhu)
- `Resources/source/design.png` – původní návrh
- `tools/make_background.py` – z návrhu odstraní ručičky/LEDky, které plugin kreslí živě
