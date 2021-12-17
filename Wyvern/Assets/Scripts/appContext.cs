using UnityEngine;
using System.Diagnostics;

using Debug = UnityEngine.Debug;

public class AppContext {

	public Figure figure;

	public AppContext() {
		figure = new Figure();
	}
}

public class ProfileTimer {
	private Stopwatch _sw = new Stopwatch();

	public ProfileTimer() {
		_sw.Start();
	}

	public void Start() {
		_sw.Restart();
	}

	public double Stop() {
		_sw.Stop();
		return _sw.Elapsed.TotalMilliseconds;
	}

	public double Stop(string msg) {
		_sw.Stop();
		PrintTime(msg);
		return _sw.Elapsed.TotalMilliseconds;
	}

	public void PrintTime(string msg) {
		double t = _sw.Elapsed.TotalMilliseconds;
		Debug.Log("[Profile timer] (" + msg + ") " + t + " ms");
	}
}