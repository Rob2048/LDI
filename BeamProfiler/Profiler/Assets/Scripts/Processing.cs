using System.Collections;
using System.Collections.Generic;
using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using UnityEngine;

public struct Centroid {
	public Vector2 position;
	public Vector2 min;
	public Vector2 max;
	public Vector2 size;
}