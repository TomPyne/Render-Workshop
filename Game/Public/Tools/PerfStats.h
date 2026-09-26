#pragma once

namespace PerfStats
{
	void RecordFrame(float DeltaSeconds);

	void BeginUpdate();
	void EndUpdate();

	void BeginRender();
	void EndRender();

	void DrawPerfWindow(bool* Open);
}
