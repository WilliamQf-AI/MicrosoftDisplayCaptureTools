﻿using CaptureCardViewer.Mocks;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Microsoft.Win32;
using MicrosoftDisplayCaptureTools.CaptureCard;
using MicrosoftDisplayCaptureTools.ConfigurationTools;
using MicrosoftDisplayCaptureTools.Display;
using MicrosoftDisplayCaptureTools.Framework;
using System;
using System.Collections.ObjectModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using System.Windows.Threading;
using Windows.Foundation;
using Windows.Graphics.Imaging;
using WinRT;

namespace CaptureCardViewer.ViewModels
{
	public partial class WorkspaceViewModel : ObservableObject
	{
		Core testFramework;
		Dispatcher dispatcher;

		[ObservableProperty]
		object activeContent;

		public ObservableCollection<ToolboxViewModel> Toolboxes { get; } = new();
		public ObservableCollection<CaptureCardViewModel> CaptureCards { get; } = new();
		public ObservableCollection<DisplayEngineViewModel> DisplayEngines { get; } = new();

		public RichTextLogger Logger { get; } = new RichTextLogger();

		public ObservableCollection<object> Documents { get; } = new();

		public WorkspaceViewModel()
		{
			testFramework = new Core(Logger);
			dispatcher = Dispatcher.CurrentDispatcher;
			var command = this.LoadFromConfigFileCommand;

			Documents.Add(this);

#if DEBUG
			CaptureCards.Add(new CaptureCardViewModel(this, new MockController()));
#endif
		}

		[ICommand]
		async void LoadFromConfigFile()
		{
			var dialog = new OpenFileDialog(); //file picker
			dialog.Title = "Load a Configuration";
			if (dialog.ShowDialog() == true)
			{
				try
				{
					await Task.Run(() =>
					{
						// Create a new framework from scratch
						var newInstance = new Core(Logger);
						newInstance.LoadConfigFile(dialog.FileName);

						// Now that the configuration was successfully loaded, update the view models
						testFramework = newInstance;
					});

					CaptureCards.Clear();
					Toolboxes.Clear();

					// For each type of plugin, intialize the view models on a background thread and
					// then add them to the ObservableCollections on the UI thread
					
					(await Task.Run(() => testFramework.GetCaptureCards()
						.Select((card) => new CaptureCardViewModel(this, card))
						.ToList()))
						.ForEach((card) => CaptureCards.Add(card));

					(await Task.Run(() => testFramework.GetConfigurationToolboxes()
						.Select((toolbox) => new ToolboxViewModel(toolbox))
						.ToList()))
						.ForEach((toolbox) => Toolboxes.Add(toolbox));

					(await Task.Run(() => testFramework.GetDisplayEngines()
						.Select((engine) => new DisplayEngineViewModel(engine))
						.ToList()))
						.ForEach((engine) => DisplayEngines.Add(engine));

				}
				catch (Exception ex)
				{
					MessageBox.Show("Failed to load configuration.\r\n" + ex.Message);
				}
			}
		}

		// Loading Display Manager file
		[ICommand]
		async void LoadDisplayEngineFromFile()
		{
			var dialog = new OpenFileDialog(); //file picker
			dialog.Title = "Load Display Engine file";
			if (dialog.ShowDialog() == true)
			{
				try
				{
					var filename = dialog.FileName.ToString();
					IDisplayEngine engine = null;
					await Task.Run(() =>
					{
						engine = testFramework.LoadDisplayEngine(filename);
					});
					DisplayEngines.Add(new DisplayEngineViewModel(engine!));
				}
				catch (Exception ex)
				{
					MessageBox.Show("Failed to load the display engine.\r\n" + ex.Message);
				}
			}
		}

		// Loading Capture Plugin file
		[ICommand]
		async void LoadCaptureCardFromFile()
		{
			var dialog = new OpenFileDialog();
			dialog.Title = "Load Capture Plugin file";
			if (dialog.ShowDialog() == true)
			{
				try
				{
					var filename = dialog.FileName.ToString();
					CaptureCardViewModel controller = null;

					await Task.Run(() =>
					{
						controller = new CaptureCardViewModel(this, testFramework.LoadCapturePlugin(filename));
					});

					CaptureCards.Add(controller!);
				}
				catch (Exception ex)
				{
					MessageBox.Show("Failed to load the capture plugin.\r\n" + ex.Message);
				}

			}
		}

		// Loading Toolbox file
		[ICommand]
		async void LoadToolboxFromFile()
		{
			var dialog = new OpenFileDialog();
			dialog.Title = "Load Toolbox file";
			if (dialog.ShowDialog() == true)
			{
				try
				{
					var filename = dialog.FileName.ToString();
					IConfigurationToolbox toolbox = null;
					await Task.Run(() =>
					{
						toolbox = testFramework.LoadToolbox(filename);
					});
					Toolboxes.Add(new ToolboxViewModel(toolbox!));
				}
				catch (Exception ex)
				{
					MessageBox.Show("Failed to load the toolbox.\r\n" + ex.Message);
				}
			}
		}

		// Apply tools to the framework's display engine
		private void ApplyToolsToEngine(MicrosoftDisplayCaptureTools.Display.IDisplayOutput displayOutput)
		{
			foreach (var toolbox in this.testFramework.GetConfigurationToolboxes())
			{
				foreach (var toolName in toolbox.GetSupportedTools())
				{
					var tool = toolbox.GetTool(toolName);

					/*if (userInput)
					{
						var suppConfig = tool.GetSupportedConfigurations();
						foreach (var config in suppConfig)
						{
							if (cbi_ref.SelectedItem != null)
							{
								ComboBoxItem cbi = (ComboBoxItem)cbi_ref.SelectedItem;
								string? sel = cbi.Content.ToString();
								if (sel == config)
								{
									tool.SetConfiguration(config);
								}

							}
							if (cbi_res.SelectedItem != null)
							{
								ComboBoxItem cbi = (ComboBoxItem)cbi_res.SelectedItem;
								string? sel = cbi.Content.ToString();
								if (sel == config)
								{
									tool.SetConfiguration(config);
								}
							}

							if (cbi_col.SelectedItem != null)
							{
								ComboBoxItem cbi = (ComboBoxItem)cbi_col.SelectedItem;
								string? sel = cbi.Content.ToString();
								if (sel == config)
								{
									tool.SetConfiguration(config);
								}
							}
						}
					}*/

					tool.Apply(displayOutput);
				}
			}
		}

	}
}
