#pragma once

// This is used to gather diagnostics and continuously print them on the
// right-hand side of the interactive terminal
//
// NOTE: this is currently not used, as tracking Rx/Tx buffers may need more
// precision than possible with Delay

void interactive_diagnostic_task();
void PrintDiagnostics();
