# Progress TUI on Named Pipe

Rendering the progress of HPC, batch, or database applications can be tricky using console or log. It could be helpful if we can render a progress bar outside from the running process. This script renders the progress of another process by receiving JSON objects. (Yes, it is very like an RPC but we do it locally via a named pipe) Since the protocol is build upon text-based objects, there is no limitation on what language the task process is written. You can easily embed a small client snippet in your application.

## Demo (Python-client)

### The code is very simple

![code_overview](figures/code_overview.gif)

### Monitor the process of one process is easy

![single_process](figures/single_process_demo.gif)

### , and easy for multi-processes as well

![multi_processes](figures/multi_process_demo.gif)

## Usage

```bash
# From the terminal you want to view the progress bars
./monitor.py
```

## Thanks

Thanks for the brilliant work of [tqdm](https://github.com/tqdm/tqdm)
