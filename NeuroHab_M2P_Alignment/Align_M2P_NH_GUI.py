"""Tkinter front-end for the NeuroHab / M2P alignment pipeline.

Wraps align_pipeline.py in a point-and-click window so the pipeline can be run
without editing paths in code. Four regions:

    - header: title bar
    - left  : controls (file pickers, output folder, options, run buttons)
    - right : data display (tabbed tables for the NH and M2P dataframes)
    - bottom: console mirroring everything the pipeline prints or warns about

No pipeline logic lives here — this module only calls align_pipeline.

Run with:  python align_gui.py
Requires align_pipeline.py to sit in the same folder.
"""

from __future__ import annotations

import queue
import sys
import threading
import traceback
import warnings
from pathlib import Path
from typing import Any, Callable

import tkinter as tk
from tkinter import filedialog, messagebox, ttk

import pandas as pd

import Align_M2P_NH_Pipeline as pipeline

# Rows rendered in a display table. The full frame is still passed to the
# pipeline; this cap only keeps the widget responsive on large files.
MAX_DISPLAY_ROWS = 1000

# --- Palette ---
BG = "#eef2f7"          # window background
SURFACE = "#ffffff"     # cards / tables
HEADER = "#1e3a5f"      # header bar
ACCENT = "#2563eb"      # primary blue
ACCENT_HOVER = "#1d4ed8"
ACCENT_LIGHT = "#dbeafe"
TEXT = "#1f2937"        # near-black slate
MUTED = "#6b7280"       # secondary text
BORDER = "#cbd5e1"
SUBTLE_ROW = "#f6f8fb"  # alternating table row
CONSOLE_BG = "#111827"
CONSOLE_FG = "#e5e7eb"

NH_FILETYPES = [("CSV files", "*.csv"), ("All files", "*.*")]
M2P_FILETYPES = [
    ("Sync files", "*.tdms *.xlsx *.xls"),
    ("TDMS files", "*.tdms"),
    ("Excel files", "*.xlsx *.xls"),
    ("All files", "*.*"),
]


class QueueWriter:
    """File-like object that forwards written text onto a queue.

    Used to capture stdout/stderr from the pipeline while it runs on a worker
    thread, so the text can be drained and displayed by the GUI thread.
    """

    def __init__(self, sink: queue.Queue) -> None:
        self._sink = sink

    # --- File-like interface ---

    def write(self, text: str) -> int:
        """Push text onto the queue.

        :param text: Text written by the caller.
        :return: Number of characters accepted.
        """
        if text:
            self._sink.put(text)
        return len(text)

    def flush(self) -> None:
        """No-op; the queue is never buffered."""
        return None


class AlignerApp:
    """Main application window for the alignment pipeline."""

    def __init__(self, root: tk.Tk) -> None:
        self._root = root
        self._output: queue.Queue = queue.Queue()

        self._nh_path = tk.StringVar()
        self._m2p_path = tk.StringVar()
        self._out_dir = tk.StringVar()
        self._drop_first_row = tk.BooleanVar(value=True)
        self._train_gap_ms = tk.StringVar(value="50")
        self._status = tk.StringVar(value="Ready.")

        self._nh_df: pd.DataFrame | None = None
        self._m2p_df: pd.DataFrame | None = None
        self._busy = False

        self._apply_theme()
        self._build_layout()
        self._poll_output()

    # --- Public interface ---

    def load_files(self) -> None:
        """Read both selected files and show them in the display tables."""
        if not self._check_ready():
            return
        self._run_in_background("Loading files", self._do_load)

    def run_alignment(self) -> None:
        """Load if needed, then run the full alignment and write the synced CSV."""
        if not self._check_ready():
            return
        self._run_in_background("Running alignment", self._do_align)

    def clear_console(self) -> None:
        """Empty the console pane."""
        self._console.configure(state="normal")
        self._console.delete("1.0", tk.END)
        self._console.configure(state="disabled")

    # --- Theme ---

    def _apply_theme(self) -> None:
        """Configure the blue-and-gray ttk styling used across the window."""
        style = ttk.Style()
        style.theme_use("clam")

        self._root.configure(background=BG)

        style.configure(".", background=BG, foreground=TEXT, font=("Segoe UI", 10))
        style.configure("TFrame", background=BG)
        style.configure("Card.TFrame", background=SURFACE, relief="flat")
        style.configure("Header.TFrame", background=HEADER)

        style.configure("TLabel", background=BG, foreground=TEXT)
        style.configure("Card.TLabel", background=SURFACE, foreground=TEXT)
        style.configure(
            "Title.TLabel", background=HEADER, foreground="#ffffff", font=("Segoe UI Semibold", 15)
        )
        style.configure(
            "Subtitle.TLabel", background=HEADER, foreground="#9db8d8", font=("Segoe UI", 9)
        )
        style.configure("Hint.TLabel", background=BG, foreground=MUTED, font=("Segoe UI", 9))
        style.configure(
            "Field.TLabel", background=SURFACE, foreground=MUTED, font=("Segoe UI Semibold", 9)
        )
        style.configure(
            "Status.TLabel", background="#dde5ef", foreground=TEXT, font=("Segoe UI", 9)
        )

        style.configure(
            "TLabelframe",
            background=SURFACE,
            bordercolor=BORDER,
            relief="solid",
            borderwidth=1,
        )
        style.configure(
            "TLabelframe.Label",
            background=SURFACE,
            foreground=ACCENT,
            font=("Segoe UI Semibold", 10),
        )

        style.configure(
            "TEntry",
            fieldbackground=SURFACE,
            bordercolor=BORDER,
            lightcolor=BORDER,
            darkcolor=BORDER,
            insertcolor=TEXT,
            padding=5,
        )
        style.map("TEntry", bordercolor=[("focus", ACCENT)], lightcolor=[("focus", ACCENT)])

        style.configure(
            "Accent.TButton",
            background=ACCENT,
            foreground="#ffffff",
            bordercolor=ACCENT,
            focuscolor=ACCENT,
            font=("Segoe UI Semibold", 10),
            padding=(10, 7),
            relief="flat",
        )
        style.map(
            "Accent.TButton",
            background=[("active", ACCENT_HOVER), ("disabled", "#9db8d8")],
            bordercolor=[("active", ACCENT_HOVER)],
        )

        style.configure(
            "Secondary.TButton",
            background="#e2e8f0",
            foreground=TEXT,
            bordercolor=BORDER,
            focuscolor="#e2e8f0",
            font=("Segoe UI", 10),
            padding=(10, 7),
            relief="flat",
        )
        style.map(
            "Secondary.TButton",
            background=[("active", "#cbd5e1"), ("disabled", "#f1f5f9")],
        )

        style.configure(
            "Browse.TButton",
            background="#e8eef7",
            foreground=ACCENT,
            bordercolor=BORDER,
            focuscolor="#e8eef7",
            font=("Segoe UI", 9),
            padding=(8, 4),
            relief="flat",
        )
        style.map("Browse.TButton", background=[("active", ACCENT_LIGHT)])

        style.configure(
            "TCheckbutton", background=SURFACE, foreground=TEXT, focuscolor=SURFACE
        )
        style.map("TCheckbutton", background=[("active", SURFACE)])

        style.configure("TNotebook", background=BG, bordercolor=BORDER, tabmargins=(0, 4, 0, 0))
        style.configure(
            "TNotebook.Tab",
            background="#dde5ef",
            foreground=MUTED,
            padding=(16, 8),
            font=("Segoe UI", 10),
            borderwidth=0,
        )
        style.map(
            "TNotebook.Tab",
            background=[("selected", SURFACE)],
            foreground=[("selected", ACCENT)],
            font=[("selected", ("Segoe UI Semibold", 10))],
        )

        style.configure(
            "Treeview",
            background=SURFACE,
            fieldbackground=SURFACE,
            foreground=TEXT,
            bordercolor=BORDER,
            rowheight=25,
            font=("Segoe UI", 9),
        )
        style.configure(
            "Treeview.Heading",
            background="#dde5ef",
            foreground=HEADER,
            font=("Segoe UI Semibold", 9),
            relief="flat",
            padding=(6, 6),
        )
        style.map(
            "Treeview.Heading",
            background=[("active", ACCENT_LIGHT)],
        )
        style.map(
            "Treeview",
            background=[("selected", ACCENT_LIGHT)],
            foreground=[("selected", TEXT)],
        )

        style.configure(
            "Vertical.TScrollbar",
            background="#dde5ef",
            troughcolor=BG,
            bordercolor=BG,
            arrowcolor=MUTED,
            relief="flat",
        )
        style.configure(
            "Horizontal.TScrollbar",
            background="#dde5ef",
            troughcolor=BG,
            bordercolor=BG,
            arrowcolor=MUTED,
            relief="flat",
        )
        style.configure(
            "TPanedwindow", background=BG
        )

    # --- Layout construction ---

    def _build_layout(self) -> None:
        """Assemble the header, panes and status bar."""
        self._root.title("NeuroHab / M2P Alignment")
        self._root.geometry("1200x800")
        self._root.minsize(950, 640)

        self._build_header()

        ttk.Label(
            self._root,
            textvariable=self._status,
            style="Status.TLabel",
            anchor="w",
            padding=(10, 5),
        ).pack(fill=tk.X, side=tk.BOTTOM)

        vertical = ttk.PanedWindow(self._root, orient=tk.VERTICAL)
        vertical.pack(fill=tk.BOTH, expand=True, padx=12, pady=10)

        horizontal = ttk.PanedWindow(vertical, orient=tk.HORIZONTAL)
        vertical.add(horizontal, weight=3)

        horizontal.add(self._build_controls(horizontal), weight=1)
        horizontal.add(self._build_display(horizontal), weight=3)
        vertical.add(self._build_console(vertical), weight=1)

    def _build_header(self) -> None:
        """Build the dark blue title bar."""
        header = ttk.Frame(self._root, style="Header.TFrame", padding=(16, 12))
        header.pack(fill=tk.X, side=tk.TOP)

        ttk.Label(header, text="NeuroHab / M2P Alignment", style="Title.TLabel").pack(anchor="w")
        ttk.Label(
            header,
            text="Align BNC event timestamps to the M2P sync clock",
            style="Subtitle.TLabel",
        ).pack(anchor="w", pady=(2, 0))

    def _build_controls(self, parent: tk.Widget) -> ttk.Frame:
        """Build the left-hand interface pane.

        :param parent: Container widget.
        :return: Frame holding the file pickers, options and action buttons.
        """
        frame = ttk.Frame(parent, padding=(0, 0, 12, 0))

        files = ttk.LabelFrame(frame, text="  Input files  ", padding=12)
        files.pack(fill=tk.X)

        self._add_file_row(
            files,
            label="NEUROHAB / BNC (.csv)",
            variable=self._nh_path,
            command=lambda: self._browse_file(
                self._nh_path, "Select the BNC CSV file", NH_FILETYPES
            ),
        )
        self._add_file_row(
            files,
            label="M2P SYNC (.tdms / .xlsx)",
            variable=self._m2p_path,
            command=lambda: self._browse_file(
                self._m2p_path, "Select the M2P sync file", M2P_FILETYPES
            ),
        )

        output = ttk.LabelFrame(frame, text="  Output  ", padding=12)
        output.pack(fill=tk.X, pady=(12, 0))

        self._add_file_row(
            output,
            label="OUTPUT FOLDER",
            variable=self._out_dir,
            command=self._browse_output_dir,
            first=True,
        )
        ttk.Label(
            output,
            text="Leave blank to save next to the input files.",
            style="Field.TLabel",
            font=("Segoe UI", 8),
        ).pack(anchor="w", pady=(4, 0))

        options = ttk.LabelFrame(frame, text="  Options  ", padding=12)
        options.pack(fill=tk.X, pady=(12, 0))

        ttk.Checkbutton(
            options,
            text="Drop first row of sync file. (Check/Uncheck on Error)",
            variable=self._drop_first_row,
        ).pack(anchor="w")

        gap = ttk.Frame(options, style="Card.TFrame")
        gap.pack(fill=tk.X, pady=(10, 0))
        ttk.Label(gap, text="TRAIN GAP (default 50 ms)", style="Field.TLabel").pack(side=tk.LEFT)
        ttk.Entry(gap, textvariable=self._train_gap_ms, width=8).pack(side=tk.LEFT, padx=(10, 0))

        actions = ttk.Frame(frame)
        actions.pack(fill=tk.X, pady=(16, 0))

        self._load_button = ttk.Button(
            actions, text="Load / Preview", style="Secondary.TButton", command=self.load_files
        )
        self._load_button.pack(fill=tk.X)

        self._run_button = ttk.Button(
            actions, text="Run Alignment", style="Accent.TButton", command=self.run_alignment
        )
        self._run_button.pack(fill=tk.X, pady=(8, 0))

        ttk.Button(
            actions, text="Clear Console", style="Secondary.TButton", command=self.clear_console
        ).pack(fill=tk.X, pady=(8, 0))

        ttk.Label(
            frame,
            text=(
                "1.  Pick the BNC CSV and the M2P sync file.\n"
                "2.  Choose an output folder (optional).\n"
                "3.  Load / Preview to check the data.\n"
                "4.  Run Alignment to write the _synced.csv\n\n"
                f"Tables show the first {MAX_DISPLAY_ROWS} rows only."
            ),
            style="Hint.TLabel",
            justify="left",
        ).pack(anchor="w", pady=(18, 0))

        return frame

    def _add_file_row(
        self,
        parent: tk.Widget,
        label: str,
        variable: tk.StringVar,
        command: Callable[[], None],
        first: bool = False,
    ) -> None:
        """Add a labelled entry plus Browse button to a container.

        :param parent: Container widget.
        :param label: Text shown above the entry.
        :param variable: Variable bound to the chosen path.
        :param command: Callback fired by the Browse button.
        :param first: True to drop the extra top padding for the first row in a group.
        """
        ttk.Label(parent, text=label, style="Field.TLabel").pack(
            anchor="w", pady=(0 if first else 12, 4)
        )
        row = ttk.Frame(parent, style="Card.TFrame")
        row.pack(fill=tk.X)
        ttk.Entry(row, textvariable=variable).pack(side=tk.LEFT, fill=tk.X, expand=True)
        ttk.Button(row, text="Browse", style="Browse.TButton", command=command).pack(
            side=tk.LEFT, padx=(8, 0)
        )

    def _build_display(self, parent: tk.Widget) -> ttk.Frame:
        """Build the right-hand display pane with one table per dataframe.

        :param parent: Container widget.
        :return: Frame holding the notebook of tables.
        """
        frame = ttk.Frame(parent)

        notebook = ttk.Notebook(frame)
        notebook.pack(fill=tk.BOTH, expand=True)

        nh_tab = ttk.Frame(notebook, style="Card.TFrame", padding=2)
        m2p_tab = ttk.Frame(notebook, style="Card.TFrame", padding=2)
        notebook.add(nh_tab, text="NeuroHab / BNC")
        notebook.add(m2p_tab, text="M2P sync")

        self._nh_table = self._build_table(nh_tab)
        self._m2p_table = self._build_table(m2p_tab)

        return frame

    def _build_table(self, parent: tk.Widget) -> ttk.Treeview:
        """Create a scrollable, empty table with alternating row shading.

        :param parent: Container widget.
        :return: The Treeview used to display a dataframe.
        """
        tree = ttk.Treeview(parent, show="headings")
        tree.tag_configure("odd", background=SUBTLE_ROW)
        tree.tag_configure("even", background=SURFACE)

        y_scroll = ttk.Scrollbar(parent, orient=tk.VERTICAL, command=tree.yview)
        x_scroll = ttk.Scrollbar(parent, orient=tk.HORIZONTAL, command=tree.xview)
        tree.configure(yscrollcommand=y_scroll.set, xscrollcommand=x_scroll.set)

        tree.grid(row=0, column=0, sticky="nsew")
        y_scroll.grid(row=0, column=1, sticky="ns")
        x_scroll.grid(row=1, column=0, sticky="ew")
        parent.rowconfigure(0, weight=1)
        parent.columnconfigure(0, weight=1)

        return tree

    def _build_console(self, parent: tk.Widget) -> ttk.LabelFrame:
        """Build the bottom console pane.

        :param parent: Container widget.
        :return: Frame holding the read-only console text widget.
        """
        frame = ttk.LabelFrame(parent, text="  Console  ", padding=(8, 6))

        self._console = tk.Text(
            frame,
            height=11,
            wrap="none",
            state="disabled",
            background=CONSOLE_BG,
            foreground=CONSOLE_FG,
            insertbackground=CONSOLE_FG,
            selectbackground="#374151",
            relief="flat",
            borderwidth=0,
            padx=10,
            pady=8,
            font=("Consolas", 10),
        )
        scroll = ttk.Scrollbar(frame, orient=tk.VERTICAL, command=self._console.yview)
        self._console.configure(yscrollcommand=scroll.set)

        self._console.tag_configure("heading", foreground="#60a5fa")
        self._console.tag_configure("error", foreground="#f87171")

        self._console.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scroll.pack(side=tk.RIGHT, fill=tk.Y)

        return frame

    # --- Actions ---

    def _browse_file(self, variable: tk.StringVar, title: str, filetypes: list) -> None:
        """Open a file chooser and store the result.

        :param variable: Variable to update with the chosen path.
        :param title: Dialog title.
        :param filetypes: Tk filetype tuples.
        """
        initial = Path(variable.get()).parent if variable.get() else Path.cwd()
        chosen = filedialog.askopenfilename(
            title=title, filetypes=filetypes, initialdir=str(initial)
        )
        if chosen:
            variable.set(chosen)

    def _browse_output_dir(self) -> None:
        """Open a folder chooser for the output directory."""
        current = self._out_dir.get() or self._nh_path.get()
        initial = Path(current).parent if current and Path(current).is_file() else (
            current or Path.cwd()
        )
        chosen = filedialog.askdirectory(
            title="Select an output folder", initialdir=str(initial), mustexist=False
        )
        if chosen:
            self._out_dir.set(chosen)

    def _check_ready(self) -> bool:
        """Verify both input paths exist and the options are valid.

        :return: True when the pipeline can be started.
        """
        if self._busy:
            return False

        for label, value in (("BNC CSV", self._nh_path.get()), ("M2P sync", self._m2p_path.get())):
            if not value:
                messagebox.showwarning("Missing file", f"Please select the {label} file.")
                return False
            if not Path(value).exists():
                messagebox.showerror("File not found", f"{label} file does not exist:\n{value}")
                return False

        if self._parse_train_gap() is None:
            return False

        return True

    def _parse_train_gap(self) -> float | None:
        """Validate the train gap entry.

        :return: The gap in milliseconds, or None when the entry is invalid.
        """
        try:
            gap = float(self._train_gap_ms.get())
        except ValueError:
            messagebox.showerror("Invalid value", "Train gap must be a number, e.g. 50")
            return None

        if gap <= 0:
            messagebox.showerror("Invalid value", "Train gap must be greater than zero.")
            return None

        return gap

    def _selected_out_dir(self) -> str | None:
        """Return the chosen output directory, or None for 'next to the inputs'.

        :return: Directory path, or None when the field is blank.
        """
        chosen = self._out_dir.get().strip()
        return chosen or None

    def _do_load(self) -> dict[str, Any]:
        """Read both input files, applying the drop-first-row option.

        :return: Result payload with both dataframes.
        :raises ValueError: If the M2P file extension is unsupported.
        """
        nh_df, m2p_df = pipeline.read_all(self._nh_path.get(), self._m2p_path.get())

        if self._drop_first_row.get():
            m2p_df = pipeline.drop_first_row(m2p_df)
            print("Dropped first row of the sync file.")

        print(f"BNC rows loaded : {len(nh_df)}")
        print(f"Sync rows loaded: {len(m2p_df)}")
        return {"nh": nh_df, "m2p": m2p_df}

    def _do_align(self) -> dict[str, Any]:
        """Load the files and run the alignment, writing the synced CSV.

        :return: Result payload with both dataframes.
        """
        loaded = self._do_load()
        nh_df, m2p_df = loaded["nh"], loaded["m2p"]

        out_dir = self._selected_out_dir()
        print(f"Output folder   : {out_dir or 'alongside input files'}")

        print("\n--- Alignment ---")
        pipeline.align_to_M2P(
            nh_df, m2p_df, self._nh_path.get(), self._m2p_path.get(), out_dir=out_dir
        )
        print("\nDone.")

        return {"nh": nh_df, "m2p": m2p_df}

    # --- Background execution ---

    def _run_in_background(self, label: str, task: Callable[[], dict[str, Any]]) -> None:
        """Run a pipeline task off the GUI thread with output captured.

        :param label: Short description shown in the status bar.
        :param task: Callable performing the work and returning a result payload.
        """
        self._set_busy(True, f"{label}...")
        self._write_console(f"\n=== {label} ===\n", tag="heading")

        def worker() -> None:
            stdout, stderr = sys.stdout, sys.stderr
            sys.stdout = sys.stderr = QueueWriter(self._output)
            try:
                with warnings.catch_warnings():
                    warnings.simplefilter("always")
                    result = task()
                self._root.after(0, lambda: self._on_success(result, label))
            except Exception:
                message = traceback.format_exc()
                self._root.after(0, lambda: self._on_failure(message))
            finally:
                sys.stdout, sys.stderr = stdout, stderr

        threading.Thread(target=worker, daemon=True).start()

    def _on_success(self, result: dict[str, Any], label: str) -> None:
        """Update the tables after a task completes.

        :param result: Payload returned by the task.
        :param label: Description used in the status message.
        """
        self._nh_df = result["nh"]
        self._m2p_df = result["m2p"]
        self._populate_table(self._nh_table, self._nh_df)
        self._populate_table(self._m2p_table, self._m2p_df)
        self._set_busy(False, f"{label} finished.")

    def _on_failure(self, message: str) -> None:
        """Report an exception raised by a task.

        :param message: Formatted traceback.
        """
        self._write_console("\n" + message, tag="error")
        self._set_busy(False, "Failed — see console.")
        messagebox.showerror("Pipeline error", message.strip().splitlines()[-1])

    def _set_busy(self, busy: bool, status: str) -> None:
        """Toggle the action buttons and update the status bar.

        :param busy: True while a task is running.
        :param status: Status bar text.
        """
        self._busy = busy
        state = "disabled" if busy else "normal"
        self._load_button.configure(state=state)
        self._run_button.configure(state=state)
        self._status.set(status)

    # --- Output plumbing ---

    def _poll_output(self) -> None:
        """Drain captured stdout/stderr into the console, then reschedule."""
        chunks = []
        try:
            while True:
                chunks.append(self._output.get_nowait())
        except queue.Empty:
            pass

        if chunks:
            self._write_console("".join(chunks))

        self._root.after(100, self._poll_output)

    def _write_console(self, text: str, tag: str | None = None) -> None:
        """Append text to the console and scroll to the bottom.

        :param text: Text to append.
        :param tag: Optional text tag controlling the colour.
        """
        self._console.configure(state="normal")
        self._console.insert(tk.END, text, tag or ())
        self._console.see(tk.END)
        self._console.configure(state="disabled")

    def _populate_table(self, tree: ttk.Treeview, df: pd.DataFrame) -> None:
        """Render a dataframe into a Treeview.

        :param tree: Table to fill.
        :param df: Dataframe to display; only the first MAX_DISPLAY_ROWS are shown.
        """
        tree.delete(*tree.get_children())

        columns = [str(c) for c in df.columns]
        tree["columns"] = columns
        for name in columns:
            tree.heading(name, text=name)
            tree.column(name, width=max(90, min(260, len(name) * 11)), anchor="w")

        for i, row in enumerate(df.head(MAX_DISPLAY_ROWS).itertuples(index=False)):
            tree.insert(
                "",
                tk.END,
                values=["" if pd.isna(v) else str(v) for v in row],
                tags=("odd" if i % 2 else "even",),
            )


def main() -> None:
    """Launch the alignment GUI."""
    root = tk.Tk()
    AlignerApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()