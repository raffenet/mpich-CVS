using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using Microsoft.Win32; // registry reading/writing
using System.Net; // Dns
using System.IO; // File
using System.Diagnostics; // Process
using System.Runtime.InteropServices;

namespace wmpiconfig
{
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class wmpiconfig : System.Windows.Forms.Form
	{
		private bool domain_populated;
		private Color setting_color;
		private Hashtable hash;
		private string smpd;
		private string mpiexec;
		private System.Windows.Forms.Label host_label;
		private System.Windows.Forms.TextBox host_textBox;
		private System.Windows.Forms.Button get_settings_button;
		private System.Windows.Forms.ListView list;
		private System.Windows.Forms.Button apply_button;
		private System.Windows.Forms.Button cancel_button;
		private System.Windows.Forms.ListView hosts_list;
		private System.Windows.Forms.Label domain_label;
		private System.Windows.Forms.Button scan_button;
		private System.Windows.Forms.ColumnHeader HostsHeader;
		private System.Windows.Forms.ColumnHeader SettingsHeader;
		private System.Windows.Forms.ColumnHeader DefaultHeader;
		private System.Windows.Forms.ColumnHeader AvailableHeader;
		private System.Windows.Forms.TextBox output_textBox;
		private System.Windows.Forms.Button ok_button;
		private System.Windows.Forms.ComboBox domain_comboBox;
		private System.Windows.Forms.Button get_hosts_button;
		private System.Windows.Forms.Button apply_all_button;
		private System.Windows.Forms.CheckBox append_checkBox;
		private System.Windows.Forms.CheckBox click_checkBox;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public wmpiconfig()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			hash = new Hashtable();
			smpd = get_smpd();
			mpiexec = get_mpiexec();
			setting_color = Color.FromArgb(204,230,230);

			domain_comboBox.Text = Environment.UserDomainName;
			domain_populated = false;
			host_textBox.Text = Environment.MachineName.ToLower();
			hosts_list.Items.Add(host_textBox.Text);

			// set defaults
			hash["log"] = new Setting("log", "", "no", "yes,no");
			hash["logfile"] = new Setting("logfile", "", "none", "");
			hash["channel"] = new Setting("channel", "", "sock", "sock,ssm,shm,sshm,ib");
			hash["internode_channel"] = new Setting("internode_channel", "", "sock", "sock,ssm,ib");
			hash["phrase"] = new Setting("phrase", "", "", "");
			hash["hosts"] = new Setting("hosts", "", "localhost", "");
			hash["max_logfile_size"] = new Setting("max_logfile_size", "", "unlimited", "");
			hash["timeout"] = new Setting("timeout", "", "infinite", "");
			//hash["map_drives"] = new Setting("map_drives", "", "no", "yes,no");
			hash["exitcodes"] = new Setting("exitcodes", "", "no", "yes,no");
			hash["port"] = new Setting("port", "", "8676", "");

			UpdateHash(get_settings(host_textBox.Text));
			UpdateListBox();
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(wmpiconfig));
			this.host_label = new System.Windows.Forms.Label();
			this.host_textBox = new System.Windows.Forms.TextBox();
			this.get_settings_button = new System.Windows.Forms.Button();
			this.list = new System.Windows.Forms.ListView();
			this.SettingsHeader = new System.Windows.Forms.ColumnHeader();
			this.DefaultHeader = new System.Windows.Forms.ColumnHeader();
			this.AvailableHeader = new System.Windows.Forms.ColumnHeader();
			this.apply_button = new System.Windows.Forms.Button();
			this.cancel_button = new System.Windows.Forms.Button();
			this.hosts_list = new System.Windows.Forms.ListView();
			this.HostsHeader = new System.Windows.Forms.ColumnHeader();
			this.domain_label = new System.Windows.Forms.Label();
			this.scan_button = new System.Windows.Forms.Button();
			this.output_textBox = new System.Windows.Forms.TextBox();
			this.ok_button = new System.Windows.Forms.Button();
			this.domain_comboBox = new System.Windows.Forms.ComboBox();
			this.get_hosts_button = new System.Windows.Forms.Button();
			this.apply_all_button = new System.Windows.Forms.Button();
			this.append_checkBox = new System.Windows.Forms.CheckBox();
			this.click_checkBox = new System.Windows.Forms.CheckBox();
			this.SuspendLayout();
			// 
			// host_label
			// 
			this.host_label.Location = new System.Drawing.Point(144, 8);
			this.host_label.Name = "host_label";
			this.host_label.Size = new System.Drawing.Size(32, 16);
			this.host_label.TabIndex = 0;
			this.host_label.Text = "Host:";
			// 
			// host_textBox
			// 
			this.host_textBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.host_textBox.Location = new System.Drawing.Point(176, 8);
			this.host_textBox.Name = "host_textBox";
			this.host_textBox.Size = new System.Drawing.Size(288, 20);
			this.host_textBox.TabIndex = 1;
			this.host_textBox.Text = "localhost";
			// 
			// get_settings_button
			// 
			this.get_settings_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.get_settings_button.Location = new System.Drawing.Point(384, 32);
			this.get_settings_button.Name = "get_settings_button";
			this.get_settings_button.Size = new System.Drawing.Size(80, 23);
			this.get_settings_button.TabIndex = 2;
			this.get_settings_button.Text = "&Get Settings";
			this.get_settings_button.Click += new System.EventHandler(this.get_settings_button_Click);
			// 
			// list
			// 
			this.list.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
				| System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.list.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
																				   this.SettingsHeader,
																				   this.DefaultHeader,
																				   this.AvailableHeader});
			this.list.FullRowSelect = true;
			this.list.GridLines = true;
			this.list.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
			this.list.Location = new System.Drawing.Point(144, 64);
			this.list.Name = "list";
			this.list.Size = new System.Drawing.Size(320, 376);
			this.list.TabIndex = 3;
			this.list.View = System.Windows.Forms.View.Details;
			this.list.ItemActivate += new System.EventHandler(this.list_ItemActivate);
			this.list.AfterLabelEdit += new System.Windows.Forms.LabelEditEventHandler(this.list_AfterLabelEdit);
			// 
			// SettingsHeader
			// 
			this.SettingsHeader.Text = "Settings";
			this.SettingsHeader.Width = 132;
			// 
			// DefaultHeader
			// 
			this.DefaultHeader.Text = "Default";
			// 
			// AvailableHeader
			// 
			this.AvailableHeader.Text = "Available options";
			this.AvailableHeader.Width = 122;
			// 
			// apply_button
			// 
			this.apply_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.apply_button.Location = new System.Drawing.Point(224, 448);
			this.apply_button.Name = "apply_button";
			this.apply_button.TabIndex = 4;
			this.apply_button.Text = "&Apply";
			this.apply_button.Click += new System.EventHandler(this.apply_button_Click);
			// 
			// cancel_button
			// 
			this.cancel_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.cancel_button.Location = new System.Drawing.Point(392, 448);
			this.cancel_button.Name = "cancel_button";
			this.cancel_button.TabIndex = 5;
			this.cancel_button.Text = "&Cancel";
			this.cancel_button.Click += new System.EventHandler(this.cancel_button_Click);
			// 
			// hosts_list
			// 
			this.hosts_list.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
				| System.Windows.Forms.AnchorStyles.Left)));
			this.hosts_list.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
																						 this.HostsHeader});
			this.hosts_list.FullRowSelect = true;
			this.hosts_list.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
			this.hosts_list.Location = new System.Drawing.Point(8, 112);
			this.hosts_list.Name = "hosts_list";
			this.hosts_list.Size = new System.Drawing.Size(128, 360);
			this.hosts_list.TabIndex = 6;
			this.hosts_list.View = System.Windows.Forms.View.Details;
			this.hosts_list.SelectedIndexChanged += new System.EventHandler(this.hosts_list_SelectedIndexChanged);
			// 
			// HostsHeader
			// 
			this.HostsHeader.Text = "Hosts";
			this.HostsHeader.Width = 107;
			// 
			// domain_label
			// 
			this.domain_label.Location = new System.Drawing.Point(8, 8);
			this.domain_label.Name = "domain_label";
			this.domain_label.Size = new System.Drawing.Size(48, 16);
			this.domain_label.TabIndex = 7;
			this.domain_label.Text = "Domain:";
			// 
			// scan_button
			// 
			this.scan_button.Location = new System.Drawing.Point(8, 80);
			this.scan_button.Name = "scan_button";
			this.scan_button.Size = new System.Drawing.Size(80, 23);
			this.scan_button.TabIndex = 9;
			this.scan_button.Text = "&Scan Hosts";
			this.scan_button.Click += new System.EventHandler(this.scan_button_Click);
			// 
			// output_textBox
			// 
			this.output_textBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.output_textBox.Location = new System.Drawing.Point(144, 32);
			this.output_textBox.Multiline = true;
			this.output_textBox.Name = "output_textBox";
			this.output_textBox.ReadOnly = true;
			this.output_textBox.Size = new System.Drawing.Size(232, 32);
			this.output_textBox.TabIndex = 10;
			this.output_textBox.Text = "";
			// 
			// ok_button
			// 
			this.ok_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.ok_button.Location = new System.Drawing.Point(312, 448);
			this.ok_button.Name = "ok_button";
			this.ok_button.Size = new System.Drawing.Size(72, 23);
			this.ok_button.TabIndex = 11;
			this.ok_button.Text = "OK";
			this.ok_button.Click += new System.EventHandler(this.ok_button_Click);
			// 
			// domain_comboBox
			// 
			this.domain_comboBox.Location = new System.Drawing.Point(8, 24);
			this.domain_comboBox.Name = "domain_comboBox";
			this.domain_comboBox.Size = new System.Drawing.Size(112, 21);
			this.domain_comboBox.TabIndex = 12;
			this.domain_comboBox.DropDown += new System.EventHandler(this.domain_comboBox_DropDown);
			// 
			// get_hosts_button
			// 
			this.get_hosts_button.Location = new System.Drawing.Point(8, 48);
			this.get_hosts_button.Name = "get_hosts_button";
			this.get_hosts_button.TabIndex = 13;
			this.get_hosts_button.Text = "&Get Hosts";
			this.get_hosts_button.Click += new System.EventHandler(this.get_hosts_button_Click);
			// 
			// apply_all_button
			// 
			this.apply_all_button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.apply_all_button.Location = new System.Drawing.Point(144, 448);
			this.apply_all_button.Name = "apply_all_button";
			this.apply_all_button.TabIndex = 14;
			this.apply_all_button.Text = "Apply All";
			this.apply_all_button.Click += new System.EventHandler(this.apply_all_button_Click);
			// 
			// append_checkBox
			// 
			this.append_checkBox.Appearance = System.Windows.Forms.Appearance.Button;
			this.append_checkBox.Location = new System.Drawing.Point(88, 51);
			this.append_checkBox.Name = "append_checkBox";
			this.append_checkBox.Size = new System.Drawing.Size(16, 16);
			this.append_checkBox.TabIndex = 15;
			this.append_checkBox.Text = "+";
			// 
			// click_checkBox
			// 
			this.click_checkBox.Appearance = System.Windows.Forms.Appearance.Button;
			this.click_checkBox.Location = new System.Drawing.Point(96, 80);
			this.click_checkBox.Name = "click_checkBox";
			this.click_checkBox.Size = new System.Drawing.Size(40, 24);
			this.click_checkBox.TabIndex = 16;
			this.click_checkBox.Text = "click";
			// 
			// wmpiconfig
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(472, 483);
			this.Controls.Add(this.cancel_button);
			this.Controls.Add(this.apply_button);
			this.Controls.Add(this.click_checkBox);
			this.Controls.Add(this.append_checkBox);
			this.Controls.Add(this.apply_all_button);
			this.Controls.Add(this.get_hosts_button);
			this.Controls.Add(this.domain_comboBox);
			this.Controls.Add(this.ok_button);
			this.Controls.Add(this.output_textBox);
			this.Controls.Add(this.scan_button);
			this.Controls.Add(this.domain_label);
			this.Controls.Add(this.hosts_list);
			this.Controls.Add(this.list);
			this.Controls.Add(this.get_settings_button);
			this.Controls.Add(this.host_textBox);
			this.Controls.Add(this.host_label);
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.Name = "wmpiconfig";
			this.Text = "MPICH2 Configurable Settings";
			this.ResumeLayout(false);

		}
		#endregion

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() 
		{
			Application.Run(new wmpiconfig());
		}

		internal void PopulateDomainList()
		{
			CompEnum ce;
			string domainName = "";
			string currentDomain = System.Environment.UserDomainName;
			int index = -1;

			Cursor.Current = Cursors.WaitCursor;

			// browse for domains
			ce = new CompEnum(0x80000000, null);
			for (int i=0; i<ce.Length; i++)
			{
				domainName = ce[i].Name;
				// Add the name to the dropdown list if it already doesn't exist
				bool found = false;
				foreach (string str in domain_comboBox.Items)
				{
					if (str.ToLower() == domainName.ToLower())
						found = true;
				}
				if (!found)
				{
					domain_comboBox.Items.Add(domainName);
				}
				// Save the index of the current machine's domain so it will be selected by default
				if (domainName.ToLower() == currentDomain.ToLower())
				{
					index = i;
				}
			}
			if (index == -1)
			{
				index = domain_comboBox.Items.Add(currentDomain);
			}
			domain_comboBox.SelectedIndex = index;
			Cursor.Current = Cursors.Default;
		}

		internal string [] GetMachines()
		{
			CompEnum ce;
			string [] results = null;

			Cursor.Current = Cursors.WaitCursor;

			ce = new CompEnum(0xFFFFFFFF, domain_comboBox.Text);
			int numServer = ce.Length;

			if (ce.LastError.Length == 0)
			{
				IEnumerator enumerator = ce.GetEnumerator();

				// add the domain text to the dropdown list if it isn't already there
				bool found = false;
				foreach (string str in domain_comboBox.Items)
				{
					if (str == domain_comboBox.Text)
					{
						found = true;
					}
				}
				if (!found)
				{
					domain_comboBox.Items.Add(domain_comboBox.Text);
				}

				results = new string[numServer];
				int i = 0;
				while (enumerator.MoveNext())
				{
					results[i] = ce[i].Name;
					i++;
				}
			}
			else
			{
				output_textBox.Text = "Error \"" + ce.LastError + "\"";
			}

			Cursor.Current = Cursors.Default;
			return results;
		}

		private void UpdateHash(Hashtable h)
		{
			// update or add entries to the internal hash for each key in the input hash
			foreach (string str in h.Keys)
			{
				if (str != "binary") // ignore the smpd binary key because it is machine specific
				{
					if (hash.Contains(str))
					{
						((Setting)hash[str])._value = (string)h[str];
					}
					else
					{
						hash[str] = new Setting(str, (string)h[str], "", "");
					}
				}
			}
			// remove settings from the internal hash for keys not in the input hash
			foreach (string str in hash.Keys)
			{
				if (!h.Contains(str))
				{
					((Setting)hash[str])._value = "";
				}
			}
		}

		private void UpdateListBox()
		{
			int i;
			Hashtable h = new Hashtable();
			// make a hash of the settings already in the listbox
			for (i=0; i<list.Items.Count; i+=2)
			{
				// add the name to the hash
				h.Add(list.Items[i].Text, i+1);
				// clear the current setting
				list.Items[i+1].Text = "";
			}
			// update or add items to the listbox
			foreach (string str in hash.Keys)
			{
				if (h.Contains(str))
				{
					i = (int)h[str];
					list.Items[i].Text = ((Setting)hash[str])._value;
				}
				else
				{
					Setting s = (Setting)hash[str];
					ListViewItem item = list.Items.Add(str);
					item.BackColor = setting_color;
					item = list.Items.Add(s._value);
					item.SubItems.Add(s._default_val);
					item.SubItems.Add(s._available_values);
				}
			}
		}

		delegate void ScanHostDelegate(string host);
		private void ScanHost(string host)
		{
			string result = host + "\r\n";
			Hashtable h;
			h = get_settings(host);
			foreach (string key in h.Keys)
			{
				result = result + key + " = " + h[key] + "\r\n";
			}
			UpdateHostScanResultDelegate rd = new UpdateHostScanResultDelegate(UpdateHostScanResult);
			object [] list = new object[1] { result };
			Invoke(rd, list);
		}

		delegate void UpdateHostScanResultDelegate(string result);
		private void UpdateHostScanResult(string result)
		{
			output_textBox.AppendText(result);
		}

		private void scan_button_Click(object sender, System.EventArgs e)
		{
			foreach (ListViewItem item in hosts_list.Items)
			{
				ScanHostDelegate shd = new ScanHostDelegate(ScanHost);
				shd.BeginInvoke(item.Text, null, null);
			}
		}

		private Hashtable get_settings(string host)
		{
			string option, val;
			Hashtable hash = new Hashtable();
			Process p = new Process();

			p.StartInfo.FileName = mpiexec;
			p.StartInfo.Arguments = string.Format("-noprompt -path {{SMPD_PATH}} -n 1 -host {0} smpd.exe -enumerate", host);
			p.StartInfo.RedirectStandardOutput = true;
			p.StartInfo.CreateNoWindow = true;
			p.StartInfo.UseShellExecute = false;

			//MessageBox.Show("About to launch: " + p.StartInfo.FileName + " " + p.StartInfo.Arguments);
			try
			{
				p.Start();
				while ((option = p.StandardOutput.ReadLine()) != null)
				{
					val = p.StandardOutput.ReadLine();
					if (val == null)
						break;
					hash.Add(option, val);
				}
				p.WaitForExit();
				if (p.ExitCode != 0)
				{
					hash.Clear();
				}
				p.Close();
			}
			catch (Exception)
			{
			}
			return hash;
		}

		private string get_mpiexec()
		{
			string mpiexec = "";
			object obj;
			try
			{
				RegistryKey key = Registry.LocalMachine.OpenSubKey(@"Software\MPICH2");
				if (key != null)
				{
					obj = key.GetValue("Path");
					key.Close();
					if (obj != null)
					{
						mpiexec = obj.ToString();
						if (mpiexec.EndsWith(@"\"))
						{
							mpiexec = mpiexec + @"bin\mpiexec.exe";
						}
						else
						{
							mpiexec = mpiexec + @"\bin\mpiexec.exe";
						}
						if (!File.Exists(mpiexec))
						{
							mpiexec = "";
						}
					}
				}
				if (mpiexec == "")
				{
					key = Registry.LocalMachine.OpenSubKey(@"Software\MPICH\SMPD");
					if (key != null)
					{
						obj = key.GetValue("binary");
						key.Close();
						if (obj != null)
						{
							mpiexec = obj.ToString().Replace("smpd.exe", "mpiexec.exe");
							if (!File.Exists(mpiexec))
							{
								mpiexec = "";
							}
						}
					}
				}
				if (mpiexec == "")
				{
					mpiexec = "mpiexec.exe";
				}
				mpiexec = mpiexec.Trim();
				/*
				if (mpiexec.IndexOf(' ') != -1)
				{
					mpiexec = "\"" + mpiexec + "\"";
				}
				*/
			}
			catch (Exception)
			{
				mpiexec = "mpiexec.exe";
			}
			return mpiexec;
		}

		private string get_smpd()
		{
			string smpd = "";
			object obj;
			try
			{
				RegistryKey key = Registry.LocalMachine.OpenSubKey(@"Software\MPICH2");
				if (key != null)
				{
					obj = key.GetValue("Path");
					key.Close();
					if (obj != null)
					{
						smpd = obj.ToString();
						if (smpd.EndsWith(@"\"))
						{
							smpd = smpd + @"bin\smpd.exe";
						}
						else
						{
							smpd = smpd + @"\bin\smpd.exe";
						}
						if (!File.Exists(smpd))
						{
							smpd = "";
						}
					}
				}
				if (smpd == "")
				{
					key = Registry.LocalMachine.OpenSubKey(@"Software\MPICH\SMPD");
					if (key != null)
					{
						obj = key.GetValue("binary");
						key.Close();
						if (obj != null)
						{
							smpd = obj.ToString();
							if (!File.Exists(smpd))
							{
								smpd = "";
							}
						}
					}
				}
				if (smpd == "")
				{
					smpd = "smpd.exe";
				}
				smpd = smpd.Trim();
				/*
				if (smpd.IndexOf(' ') != -1)
				{
					smpd = "\"" + smpd + "\"";
				}
				*/
			}
			catch (Exception)
			{
				smpd = "smpd.exe";
			}
			return smpd;
		}

		private string get_value(string key)
		{
			object obj;
			try
			{
				RegistryKey regkey = Registry.LocalMachine.OpenSubKey(@"Software\MPICH\SMPD");
				obj = regkey.GetValue(key);
				regkey.Close();
				if (obj != null)
				{
					return obj.ToString();
				}
			}
			catch (Exception)
			{
			}
			return "";
		}

		private void get_settings_button_Click(object sender, System.EventArgs e)
		{
			Hashtable h;
			Cursor.Current = Cursors.WaitCursor;
			h = get_settings(host_textBox.Text);
			UpdateHash(h);
			UpdateListBox();
			add_host_to_list(host_textBox.Text);
			Cursor.Current = Cursors.Default;
		}

		private void add_host_to_list(string host)
		{
			bool found = false;
			foreach (ListViewItem item in hosts_list.Items)
			{
				if (item.Text == host)
					found = true;
			}
			if (!found)
			{
				hosts_list.Items.Add(host);
			}
		}

		private void apply_button_Click(object sender, System.EventArgs e)
		{
		}

		private void apply_all_button_Click(object sender, System.EventArgs e)
		{
		}

		private void cancel_button_Click(object sender, System.EventArgs e)
		{
			Close();
		}

		private void list_ItemActivate(object sender, System.EventArgs e)
		{
			int index;
			if (list.SelectedIndices.Count > 0)
			{
				index = list.SelectedIndices[0];
				if ((index & 0x1) == 0)
				{
					index++;
				}
				// turn on and begin editing the value field
				list.LabelEdit = true;
				list.Items[index].BeginEdit();
			}
		}

		private void list_AfterLabelEdit(object sender, System.Windows.Forms.LabelEditEventArgs e)
		{
			// turn off editing after a field has been modified to prevent the name from being modified
			list.LabelEdit = false;
		}

		private void ok_button_Click(object sender, System.EventArgs e)
		{
			Close();
		}

		private void hosts_list_SelectedIndexChanged(object sender, System.EventArgs e)
		{
			if (hosts_list.SelectedItems.Count > 0)
			{
				host_textBox.Text = hosts_list.SelectedItems[0].Text;
				if (click_checkBox.Checked)
				{
					get_settings_button_Click(null, null);
				}
			}
		}

		private void domain_comboBox_DropDown(object sender, System.EventArgs e)
		{
			if (!domain_populated)
			{
				PopulateDomainList();
				domain_populated = true;
			}
		}

		private void get_hosts_button_Click(object sender, System.EventArgs e)
		{
			string [] hosts;
			if (domain_comboBox.Text.ToLower() == Environment.MachineName.ToLower())
			{
				hosts = new string[1] { Environment.MachineName };
			}
			else
			{
				hosts = GetMachines();
			}
			if (hosts != null)
			{
				if (!append_checkBox.Checked)
				{
					hosts_list.Items.Clear();
				}
				foreach (string s in hosts)
				{
					bool found = false;
					foreach (ListViewItem item in hosts_list.Items)
					{
						if (item.Text.ToLower() == s.ToLower())
							found = true;
					}
					if (!found)
					{
						hosts_list.Items.Add(s);
					}
				}
			}
		}
	}

	public class Setting
	{
		public string _name;
		public string _value;
		public string _default_val;
		public string _available_values;

		public Setting()
		{
		}

		public Setting(string name, string val, string default_val, string available_values)
		{
			_name = name;
			_value = val;
			_default_val = default_val;
			_available_values = available_values;
		}
	}

	public class CompEnum : IEnumerable, IDisposable
	{
		#region "Server type enumeration"
		// Possible types of servers
		[FlagsAttribute]
			public enum ServerType : uint
		{
			/// <summary>
			/// All workstations
			/// </summary>
			SV_TYPE_WORKSTATION   = 0x00000001,
			/// <summary>
			/// All computers that have the server service running
			/// </summary>
			SV_TYPE_SERVER    = 0x00000002,
			/// <summary>
			/// Any server running Microsoft SQL Server
			/// </summary>
			SV_TYPE_SQLSERVER   = 0x00000004,
			/// <summary>
			/// Primary domain controller
			/// </summary>
			SV_TYPE_DOMAIN_CTRL   = 0x00000008,
			/// <summary>
			/// Backup domain controller
			/// </summary>
			SV_TYPE_DOMAIN_BAKCTRL  = 0x00000010,
			/// <summary>
			/// Server running the Timesource service
			/// </summary>
			SV_TYPE_TIME_SOURCE   = 0x00000020,
			/// <summary>
			/// Apple File Protocol servers
			/// </summary>
			SV_TYPE_AFP     = 0x00000040,
			/// <summary>
			/// Novell servers
			/// </summary>
			SV_TYPE_NOVELL    = 0x00000080,
			/// <summary>
			/// LAN Manager 2.x domain member
			/// </summary>
			SV_TYPE_DOMAIN_MEMBER  = 0x00000100,
			/// <summary>
			/// Server sharing print queue
			/// </summary>
			SV_TYPE_PRINTQ_SERVER  = 0x00000200,
			/// <summary>
			/// Server running dial-in service
			/// </summary>
			SV_TYPE_DIALIN_SERVER  = 0x00000400,
			/// <summary>
			/// Xenix server
			/// </summary>
			SV_TYPE_XENIX_SERVER  = 0x00000800,
			/// <summary>
			/// Windows NT workstation or server
			/// </summary>
			SV_TYPE_NT     = 0x00001000,
			/// <summary>
			/// Server running Windows for Workgroups
			/// </summary>
			SV_TYPE_WFW     = 0x00002000,
			/// <summary>
			/// Microsoft File and Print for NetWare
			/// </summary>
			SV_TYPE_SERVER_MFPN   = 0x00004000,
			/// <summary>
			/// Server that is not a domain controller
			/// </summary>
			SV_TYPE_SERVER_NT   = 0x00008000,
			/// <summary>
			/// Server that can run the browser service
			/// </summary>
			SV_TYPE_POTENTIAL_BROWSER = 0x00010000,
			/// <summary>
			/// Server running a browser service as backup
			/// </summary>
			SV_TYPE_BACKUP_BROWSER  = 0x00020000,
			/// <summary>
			/// Server running the master browser service
			/// </summary>
			SV_TYPE_MASTER_BROWSER  = 0x00040000,
			/// <summary>
			/// Server running the domain master browser
			/// </summary>
			SV_TYPE_DOMAIN_MASTER  = 0x00080000,
			/// <summary>
			/// Windows 95 or later
			/// </summary>
			SV_TYPE_WINDOWS    = 0x00400000,
			/// <summary>
			/// Root of a DFS tree
			/// </summary>
			SV_TYPE_DFS     = 0x00800000,
			/// <summary>
			/// Terminal Server
			/// </summary>
			SV_TYPE_TERMINALSERVER  = 0x02000000,
			/// <summary>
			/// Server clusters available in the domain
			/// </summary>
			SV_TYPE_CLUSTER_NT   = 0x01000000,
			/// <summary>
			/// Cluster virtual servers available in the domain
			/// (Not supported for Windows 2000/NT)
			/// </summary>			
			SV_TYPE_CLUSTER_VS_NT  = 0x04000000,
			/// <summary>
			/// IBM DSS (Directory and Security Services) or equivalent
			/// </summary>
			SV_TYPE_DCE     = 0x10000000,
			/// <summary>
			/// Return list for alternate transport
			/// </summary>
			SV_TYPE_ALTERNATE_XPORT  = 0x20000000,
			/// <summary>
			/// Return local list only
			/// </summary>
			SV_TYPE_LOCAL_LIST_ONLY  = 0x40000000,
			/// <summary>
			/// Lists available domains
			/// </summary>
			SV_TYPE_DOMAIN_ENUM		=  0x80000000
		}
		#endregion
		
		// Holds computer information
		[StructLayoutAttribute(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
			internal struct SERVER_INFO_101
		{
			public int sv101_platform_id;
			public string sv101_name;
			public int sv101_version_major;
			public int sv101_version_minor;
			public int sv101_type;
			public string sv101_comment;
		}

		// enumerates network computers
		[DllImport("Netapi32", CharSet=CharSet.Unicode)]
		private static extern int NetServerEnum( 
			string servername,		// must be null
			int level,				// 100 or 101
			out IntPtr bufptr,		// pointer to buffer receiving data
			int prefmaxlen,			// max length of returned data
			out int entriesread,	// num entries read
			out int totalentries,	// total servers + workstations
			uint servertype,		// server type filter
			[MarshalAs(UnmanagedType.LPWStr)]
			string domain,			// domain to enumerate
			IntPtr resume_handle );
   
		// Frees buffer created by NetServerEnum
		[DllImport("netapi32.dll")]
		private extern static int NetApiBufferFree( 
			IntPtr buf );

		// Constants
		private const int ERROR_ACCESS_DENIED = 5;
		private const int ERROR_MORE_DATA = 234;
		private const int ERROR_NO_SERVERS = 6118;

		private NetworkComputers[] _computers;
		private string _lastError = "";

		/// <summary>
		/// Converts ServerType to its underlying value
		/// </summary>
		/// <param name="serverType">One of the ServerType values</param>
		/// <param name="domain">The domain to search for computers in</param>
		public CompEnum(ServerType serverType, string domain) : this(UInt32.Parse(Enum.Format(typeof(ServerType), serverType, "x"), System.Globalization.NumberStyles.HexNumber), domain)
		{
			
		}		
		/// <summary>
		/// Populates with broadcasting computers.
		/// </summary>
		/// <param name="serverType">Server type filter</param>
		/// <param name="domain">The domain to search for computers in</param>
		public CompEnum(uint serverType, string domainName)
		{			
			int entriesread;  // number of entries actually read
			int totalentries; // total visible servers and workstations
			int result;		  // result of the call to NetServerEnum

			// Pointer to buffer that receives the data
			IntPtr pBuf = IntPtr.Zero;
			Type svType = typeof(SERVER_INFO_101);

			// structure containing info about the server
			SERVER_INFO_101 si;
			
			try
			{
				result = NetServerEnum(
					null,
					101,
					out pBuf,
					-1, 
					out entriesread,
					out totalentries,
					serverType,
					domainName,
					IntPtr.Zero);

				// Successful?
				if(result != 0) 
				{
					switch (result)
					{
						case ERROR_MORE_DATA:
							_lastError = "More data is available";
							break;
						case ERROR_ACCESS_DENIED:
							_lastError = "Access was denied";
							break;
						case ERROR_NO_SERVERS:
							_lastError = "So servers available for this domain";
							break;
						default:
							_lastError = "Unknown error code "+result;
							break;
					}					
					return;
				}
				else
				{
					_computers = new NetworkComputers[entriesread];

					int tmp = (int)pBuf;
					for(int i = 0; i < entriesread; i++ )
					{
						// fill our struct
						si = (SERVER_INFO_101)Marshal.PtrToStructure((IntPtr)tmp, svType);
						_computers[i] = new NetworkComputers(si);

						// next struct
						tmp += Marshal.SizeOf(svType);
					}
				}
			}
			finally
			{
				// free the buffer
				NetApiBufferFree(pBuf); 
				pBuf = IntPtr.Zero;
			}
		}

		/// <summary>
		/// Total computers in the collection
		/// </summary>
		public int Length
		{
			get
			{
				if(_computers!=null)
				{
					return _computers.Length;
				}
				else
				{
					return 0;
				}
			}
		}
		
		/// <summary>
		/// Last error message
		/// </summary>
		public string LastError
		{
			get { return _lastError; }
		}
		
		/// <summary>
		/// Obtains the enumerator for ComputerEnumerator class
		/// </summary>
		/// <returns>IEnumerator</returns>
		public IEnumerator GetEnumerator()
		{
			return new ComputerEnumerator(_computers);		
		}

		// cleanup
		public void Dispose()
		{
			_computers = null;	
		}
   
		public NetworkComputers this[int item]
		{
			get 
			{ 
				return _computers[item];
			}
		}

		// holds computer info.
		public struct NetworkComputers
		{
			CompEnum.SERVER_INFO_101 _computerinfo;
			internal NetworkComputers(CompEnum.SERVER_INFO_101 info)
			{
				_computerinfo = info;
			}

			/// <summary>
			/// Name of computer
			/// </summary>
			public string Name
			{
				get { return _computerinfo.sv101_name; }
			}
			/// <summary>
			/// Server comment
			/// </summary>
			public string Comment
			{
				get { return _computerinfo.sv101_comment; }
			}
			/// <summary>
			/// Major version number of OS
			/// </summary>
			public int VersionMajor
			{
				get { return _computerinfo.sv101_version_major; }
			}
			/// <summary>
			/// Minor version number of OS
			/// </summary>
			public int VersionMinor
			{
				get { return _computerinfo.sv101_version_minor; }
			}
		}

		/// <summary>
		/// Enumerates the collection of computers
		/// </summary>
		public class ComputerEnumerator : IEnumerator
		{
			private NetworkComputers[] aryComputers;
			private int indexer;

			internal ComputerEnumerator(NetworkComputers[] networkComputers)
			{
				aryComputers = networkComputers;
				indexer = -1;
			}

			public void Reset()
			{
				indexer = -1;
			}

			public object Current
			{
				get
				{
					return aryComputers[indexer];
				}
			}
			public bool MoveNext()
			{
				if(aryComputers == null)
					return false;
				if (indexer < aryComputers.Length)
					indexer++;
				return (!(indexer == aryComputers.Length));
			}
		}
	}
}
