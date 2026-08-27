# NeuroHab / M2P Alignment

Aligns NeuroHab (BNC) event timestamps — and optionally a FED3 log — to the M2P sync clock.

Two files:

| File | What it is |
| --- | --- |
| `Align_M2P_NH_Pipeline.py` | The alignment logic. Can be run on its own. |
| `Align_M2P_NH_GUI.py` | A point-and-click window that runs the pipeline. |

Both files must sit in the **same folder**. The GUI imports the pipeline by name, so don't rename either one.

---

## Setup (once per computer)

Install Python 3.10 or newer, then open a terminal and run:

```
pip install pandas npTDMS openpyxl
```

`openpyxl` is only needed if your sync files are `.xlsx` instead of `.tdms`.

---

## Using the GUI

Double-click `Align_M2P_NH_GUI.py`, or from a terminal in that folder:

```
python Align_M2P_NH_GUI.py
```

Then:

1. **NeuroHab / BNC** — Browse to the BNC `.csv`.
2. **M2P sync** — Browse to the `.tdms` (or `.xlsx`) sync file.
3. **FED3** — optional. Leave blank if you aren't aligning FED3 data. "Clear FED3" empties the field.
4. **Output folder** — optional. Blank saves the results next to the input files.
5. **Load / Preview** — reads the files and shows them in the tabs on the right. Nothing is written yet.
6. **Run Alignment** — does the alignment and writes the synced CSVs.

Everything the pipeline reports appears in the console at the bottom. Read it after every run — that's where mismatches show up.

### Options

**Drop first row of sync file** (on by default) discards the first pulse in the sync file, which is usually a spurious startup pulse. If the pulse counts come out one off, toggle this and re-run.

**Train gap** (default 50 ms) is the silent gap that separates one pulse train from the next. Pulses closer together than this are treated as one train. Only change it if the console reports a train count that doesn't match your event count.

---

## Output

For inputs `BNC_1.csv` and `FED3_1.csv`, you get:

- `BNC_1_synced.csv` — your BNC data plus a `synced_time` column
- `FED3_1_synced.csv` — your FED3 data plus a `synced_time` column (only if a FED3 file was selected)

`synced_time` is the M2P clock time of the pulse train for that event. The sync file itself isn't rewritten.

Filenames come out uppercase. That's cosmetic and doesn't affect the contents.

---

## Reading the console

A clean run looks roughly like:

```
PULSES ARE EQUAL: sumPulses: 412, numRows: 412
BNC events : 87
Sync trains: 87
Total pulses received: 412
No mismatch found in the 87 events.
NeuroHab FED3 events: 34
FED3 log rows       : 34
FED3 clock offset : 0 days 00:00:12.004
FED3 offset spread: 0.006 s
```

What to watch for:

**Pulse count mismatch** — the BNC recorded more pulses than the sync received, meaning pulses were dropped. The console lists which event rows are short and by how many.

**BNC events != sync trains** — the train grouping doesn't match the event list. Usually the train gap needs adjusting, or the drop-first-row setting is wrong.

**FED3 offset spread** — this should be small (well under a second). The FED3's own clock runs at a fixed offset from the sync clock, so the difference should be near-constant across the session. A large spread means the FED3 rows and the NeuroHab FED3 events aren't matching up one-to-one — usually the two files cover different sessions, or the FED3 event codes need adjusting (see below).

---

## Running without the GUI

Open `Align_M2P_NH_Pipeline.py`, edit the block at the bottom under `if __name__ == "__main__":`, and run `python Align_M2P_NH_Pipeline.py`.

```python
dir_path = Path().cwd().joinpath("to-sync/8-26-26 1")

NH_path = dir_path.joinpath("BNC_1.csv")
M2P_path = dir_path.joinpath("SignalSync_4.tdms")

FED3_path = None          # or dir_path.joinpath("FED3_1.csv")
OUT_DIR = None            # or a folder path
DROP_FIRST_ROW = True
```

---

## How the alignment works

The sync file is a stream of individual pulses. Pulses arriving close together form a *train*, and each train corresponds to one BNC event — the number of pulses in the train encodes which event it was. The pipeline groups pulses into trains, matches the *n*th train to the *n*th BNC event row, and stamps that event with the train's start time.

FED3 rides on the same result. Every NeuroHab row whose event is a FED3 event already has a synced time, so the *n*th FED3 event in the NeuroHab log is matched to the *n*th row of the FED3 log.

---

## Adjusting FED3 event codes

If your FED3 firmware logs different codes, edit the constant near the top of the pipeline:

```python
FED3_EVENTS = ['RP', 'LP', 'DISP', 'RETR']
```

These must match the values in the BNC file's `event` column. If the FED3 row count in the console doesn't match your FED3 file, this list is the first thing to check.

---

## Troubleshooting

**`ModuleNotFoundError: No module named 'Align_M2P_NH_Pipeline'`** — the two scripts aren't in the same folder, or one was renamed.

**`ModuleNotFoundError: No module named 'nptdms'` / `'pandas'`** — the setup step above hasn't been run, or was run against a different Python installation.

**`Unsupported M2P file format`** — the sync file isn't `.tdms`, `.xlsx`, or `.xls`.

**The window freezes** — it shouldn't; alignment runs in the background and the buttons grey out while it works. If it does freeze, the console will still have whatever was printed before the hang.

Any error produces a popup and a full red traceback in the console. The last line of that traceback is the useful part.
