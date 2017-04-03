﻿using System;
using System.IO.Ports;
using System.Threading;

namespace monitor
{
	public class Serial
	{
		Monitor monitor;
		SerialPort port;
		Thread reader;
		volatile bool shouldTerminate = false;



		public Serial(Monitor monitor, string portAddress, int baudRate)
		{
			this.monitor = monitor;

			port = new SerialPort(portAddress, baudRate);
		}



		/*
		 * Start the thread reading the data from the serial port,
		 * then it triggers and update of the UI
		 */
		public void startReading()
		{
			shouldTerminate = false;

			reader = new Thread(delegate ()
			{
				port.ReadTimeout = 1000;
				int i = 1;
				// Try to open the port every second
				while (!port.IsOpen && !shouldTerminate)
				{
					try
					{
						if (!openPort())
						{
							Thread.Sleep(1000 * i);
							i++;
						}
						else
							Console.WriteLine("--- PORT OPEN, READING... ---");
					}
					catch (ThreadInterruptedException) { }
				}

				// The port is open, read
				while (!shouldTerminate)
				{
					try
					{
						string msg = port.ReadLine();
						Console.WriteLine("--> RECEIVED MESSAGE. LENGTH: " + msg.Length + " B\n" + msg);

						if (!handleMessage(msg))
							Console.WriteLine("Received corrupted or unknown message");
						else 
							Console.WriteLine("Received valid message"); //TODO RM
					}
					catch (TimeoutException) { }
				}

				Console.WriteLine("Thread terminating...");
				closePort();
			});

			reader.IsBackground = true;
			reader.Start();
		}

		/*
		 * Terminate the thread reading data from the serial port (and close the port).
		 * Should NOT be called from withing the reader thread.
		 */
		public void stopReading()
		{
			if (reader != null)
			{
				shouldTerminate = true;
				reader.Interrupt();
				reader.Join();
				reader = null;
				Console.WriteLine("Serial port closed.");
			}
			else
			{
				Console.WriteLine("Cannot run stopReading() before startReading()");
			}
		}



		bool openPort()
		{
			if (!port.IsOpen)
			{
				try
				{
					port.Open();
				}
				catch (System.IO.IOException)
				{
					Console.WriteLine("Serial port unavailable");
				}
			}
			return port.IsOpen;
		}

		void closePort()
		{
			port.Close();
		}



		/*
		 * Decode the message and update the monitor
		 */
		bool handleMessage(string msg)
		{
			Type msgType = (Type)Convert.ToInt32(msg.Substring(0, 1));
			switch (msgType)
			{
				case Type.Info:
					return handleInfoMessage(msg);
				case Type.FrequencyLeft:
				case Type.FrequencyFront:
				case Type.FrequencyRight:
					return handleFrequencyMessage(msg);
				default:
					return false;
			}
		}

		bool handleInfoMessage(string msg)
		{
			if (msg.Length == 24)
			{
				RoadID roadID = (RoadID)Convert.ToInt32(msg.Substring(1, 1));

				if (Enum.IsDefined(typeof(RoadID), roadID))
				{
					int orientation = Convert.ToInt32(msg.Substring(18, 3).Trim());

					string manufacturer = msg.Substring(2, 8).Trim(); //TODO: better to specify the type (partial, complete) in the message format
					if (manufacturer != "")
					{
						// Complete info message
						string model = msg.Substring(10, 8).Trim();
						Priority priority = (Priority)Convert.ToInt32(msg.Substring(21, 1));
						Action requestedAction = (Action)Convert.ToInt32(msg.Substring(22, 1));
						Action currentAction = (Action)Convert.ToInt32(msg.Substring(23, 1));

						if (Enum.IsDefined(typeof(Priority), priority) ||
							Enum.IsDefined(typeof(Action), priority) ||
							Enum.IsDefined(typeof(Action), priority))
						{
							monitor.UpdateRoad(roadID, orientation, manufacturer, model,
											   priority, requestedAction, currentAction);
						}
					}
					else
					{
						// partial info message
						monitor.UpdateRoad(roadID, orientation);
					}
				}
			}

			return false;
		}

		bool handleFrequencyMessage(string msg)
		{
			if (msg.Length >= 130 && msg.Length <= 395)
			{
				// TODO: start the python script?
				return true;
			}

			return false;
		}
	}
}