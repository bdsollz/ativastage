---
name: Studio Precision
colors:
  surface: '#10131a'
  surface-dim: '#10131a'
  surface-bright: '#363941'
  surface-container-lowest: '#0b0e15'
  surface-container-low: '#191b23'
  surface-container: '#1d2027'
  surface-container-high: '#272a31'
  surface-container-highest: '#32353c'
  on-surface: '#e1e2ec'
  on-surface-variant: '#c2c6d6'
  inverse-surface: '#e1e2ec'
  inverse-on-surface: '#2e3038'
  outline: '#8c909f'
  outline-variant: '#424754'
  surface-tint: '#adc6ff'
  primary: '#adc6ff'
  on-primary: '#002e6a'
  primary-container: '#4d8eff'
  on-primary-container: '#00285d'
  inverse-primary: '#005ac2'
  secondary: '#ffb95f'
  on-secondary: '#472a00'
  secondary-container: '#ee9800'
  on-secondary-container: '#5b3800'
  tertiary: '#ffb3ad'
  on-tertiary: '#68000a'
  tertiary-container: '#ff5451'
  on-tertiary-container: '#5c0008'
  error: '#ffb4ab'
  on-error: '#690005'
  error-container: '#93000a'
  on-error-container: '#ffdad6'
  primary-fixed: '#d8e2ff'
  primary-fixed-dim: '#adc6ff'
  on-primary-fixed: '#001a42'
  on-primary-fixed-variant: '#004395'
  secondary-fixed: '#ffddb8'
  secondary-fixed-dim: '#ffb95f'
  on-secondary-fixed: '#2a1700'
  on-secondary-fixed-variant: '#653e00'
  tertiary-fixed: '#ffdad7'
  tertiary-fixed-dim: '#ffb3ad'
  on-tertiary-fixed: '#410004'
  on-tertiary-fixed-variant: '#930013'
  background: '#10131a'
  on-background: '#e1e2ec'
  surface-variant: '#32353c'
typography:
  display-lg:
    fontFamily: Inter
    fontSize: 48px
    fontWeight: '700'
    lineHeight: 56px
    letterSpacing: -0.02em
  headline-lg:
    fontFamily: Inter
    fontSize: 32px
    fontWeight: '600'
    lineHeight: 40px
    letterSpacing: -0.01em
  headline-md:
    fontFamily: Inter
    fontSize: 24px
    fontWeight: '600'
    lineHeight: 32px
  title-lg:
    fontFamily: Inter
    fontSize: 20px
    fontWeight: '600'
    lineHeight: 28px
  title-md:
    fontFamily: Inter
    fontSize: 16px
    fontWeight: '600'
    lineHeight: 24px
  body-lg:
    fontFamily: Inter
    fontSize: 16px
    fontWeight: '400'
    lineHeight: 24px
  body-md:
    fontFamily: Inter
    fontSize: 14px
    fontWeight: '400'
    lineHeight: 20px
  label-md:
    fontFamily: Inter
    fontSize: 12px
    fontWeight: '600'
    lineHeight: 16px
    letterSpacing: 0.05em
  label-sm:
    fontFamily: Inter
    fontSize: 10px
    fontWeight: '700'
    lineHeight: 12px
    letterSpacing: 0.08em
  mono-md:
    fontFamily: JetBrains Mono
    fontSize: 14px
    fontWeight: '400'
    lineHeight: 20px
rounded:
  sm: 0.25rem
  DEFAULT: 0.5rem
  md: 0.75rem
  lg: 1rem
  xl: 1.5rem
  full: 9999px
spacing:
  unit: 4px
  gutter: 16px
  margin-page: 24px
  panel-padding: 12px
  stack-gap: 8px
---

## Brand & Style

This design system is engineered for the high-stakes environment of live audiovisual production. The brand personality is professional, authoritative, and dependable, mirroring the reliability of studio-grade hardware. It prioritizes utility and immediate recognition over decorative elements.

The aesthetic follows a **High-Contrast Corporate Modern** approach. It utilizes a deep, monochromatic base to minimize light spill in darkened control rooms while using high-chroma accent colors to communicate critical system states. The interface relies on structural clarity, utilizing defined panel borders and intentional whitespace to organize complex data sets and multi-track timelines without the visual clutter often found in consumer-grade software.

## Colors

The color palette is optimized for dark environments and rapid state-identification. 

- **Background & Surfaces:** A tiered system of deep charcoals (Neutral 950 to 800) creates depth without losing the "true black" required for low-light environments.
- **Actionable States:** 
  - **Live (Red #EF4444):** Reserved exclusively for "on-air" or destructive actions. High urgency.
  - **Preview (Amber #F59E0B):** Used for staged content that is not yet live.
  - **Active/Primary (Blue #3B82F6):** Indicates selection, focus, and standard primary interactions.
  - **Success/Executing (Green #10B981):** Indicates active playback or successful command execution.
- **Contrast:** Maintain a minimum 7:1 contrast ratio for all status labels against background surfaces to ensure legibility under stressful operating conditions.

## Typography

**Inter** is the standard for its exceptional legibility and neutral tone. For timecodes and technical data (SMPTE, coordinates, hex codes), use **JetBrains Mono** to ensure character alignment and readability of numerical strings.

- **Scale:** High contrast between levels. Titles use semi-bold (600) to stand out against dark surfaces.
- **Labels:** Small labels use all-caps with increased letter spacing to provide a "hardware-stamped" look and maintain clarity at small sizes.
- **Mobile:** Scale large display types down by 20% on mobile devices, but maintain the 14px minimum for body text to ensure field usability.

## Layout & Spacing

The layout is based on a **Fluid Grid** with a 4px baseline unit. In a professional AV context, density is often required to keep all controls visible at once.

- **Panels:** The UI is composed of "Panels" separated by 1px borders. Avoid heavy gaps; use the 16px gutter primarily for separating logical clusters.
- **Density:** Use a "Comfortable" density for setup views and a "Compact" density for live performance dashboards.
- **Breakpoints:** 
  - **Desktop (1280px+):** 12-column grid. Panels are modular and can be resized.
  - **Tablet (768px - 1279px):** 8-column grid. Secondary panels (Media Library) collapse into drawers.
  - **Mobile (Under 768px):** Single column. Focus on transport controls and emergency "Blackout" buttons.

## Elevation & Depth

This design system avoids traditional shadows to prevent visual "muddiness" in dark modes. Depth is instead conveyed through **Tonal Layering and Borders**.

- **Level 0 (Background):** #0A0A0A. The base canvas.
- **Level 1 (Panels/Containers):** #171717. Raised containers. Use a 1px solid border of #262626 to define edges.
- **Level 2 (Popovers/Modals):** #262626. These use a subtle 8px blur background and a slightly brighter border (#404040) to appear "closer" to the user.
- **Active States:** Instead of a shadow, an active panel may use a 1px border of the Primary Blue (#3B82F6) to denote focus.

## Shapes

The shape language is precise and disciplined. 

- **Standard Elements:** Buttons, inputs, and small panels use a **0.5rem (8px)** radius. This softens the technical feel without appearing overly consumer-oriented.
- **Containers:** Large workspace areas or the main viewport use **1rem (16px)** radius only when floating; if docked, they are sharp-edged to maximize screen real estate.
- **Interactive Indicators:** Radio buttons and checkboxes remain classic but utilize the brand's accent colors for "Checked" states.

## Components

- **Buttons:** 
  - **Primary:** Solid Blue background with White text. 
  - **Live/Action:** Solid Red background. Pulsing animation (opacity 0.8 to 1.0) when "On Air".
  - **Ghost:** Border only, used for secondary toolbars.
- **Input Fields:** Darker than the surface (#0A0A0A), with a 1px border. The cursor and focus border should use Primary Blue.
- **Status Chips:** Small, high-contrast pills. 
  - *Example:* A "4K" chip with White text on a Grey #404040 background.
- **Transport Controls:** Large, chunky icons for Play, Pause, Stop, and Loop. 
  - **Active Playback:** Icon turns Success Green (#10B981).
- **Cards/Media Items:** Use a 16:9 aspect ratio for thumbnails. Include a progress bar overlay at the bottom for "Playing" status.
- **Lists:** High-density rows with 1px bottom dividers. Active selection uses a Blue vertical bar on the left edge (4px wide).
- **Level Meters:** Vertical or horizontal bars using a gradient scale (Green -> Amber -> Red) to show volume or CPU load.