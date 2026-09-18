using System.Diagnostics;
using System.Net.NetworkInformation;
using System.Reflection;
using System.Threading;
using SelectRegionForDbd.Core;

namespace SelectRegionForDbd
{
    public partial class MainForm : Form
    {
        private const int ZoneColumnIndex = 0;
        private const int HostColumnIndex = 1;
        private const int PingColumnIndex = 2;

        private static readonly Dictionary<string, string> PlatformExecutable = new()
        {
            ["STEAM"] = "DeadByDaylight-Win64-Shipping.exe",
            ["EGS"] = "DeadByDaylight-EGS-Shipping.exe",
            ["MS"] = "DeadByDaylight-WinGDK-Shipping.exe"
        };

        private readonly Dictionary<string, int> regionRows = [];

        private readonly System.Windows.Forms.Timer pingTimer;

        private int pingCycle;
        private int pendingPings;
        private readonly Dictionary<int, long> pings = [];

        private string? appliedZoneId;

        private string? detectedPlatform;

        public MainForm()
        {
            InitializeComponent();
            MinimumSize = new Size(
                ClientSize.Width + SystemInformation.FrameBorderSize.Width * 2,
                ClientSize.Height + SystemInformation.CaptionHeight + SystemInformation.FrameBorderSize.Height * 2);
            SetupServersGrid();
            Themes.Apply(this);
            versionLabel.Text = $"v{Assembly.GetExecutingAssembly().GetName().Version?.ToString(3)}";
            pingTimer = new System.Windows.Forms.Timer { Interval = 15000 };
            pingTimer.Tick += PingTimer_Tick;
            pingTimer.Start();
            _ = RefreshAppliedZoneAsync();
            UpdatePings();
            FilePath.Select(0, 0);
        }

        private GameliftRegion? SelectedRegion => ServersGrid.CurrentRow?.Tag as GameliftRegion;

        private string SelectedPlatform => detectedPlatform ?? "STEAM";

        // Заполнение таблицы серверов из каталога регионов
        private void SetupServersGrid()
        {
            ServersGrid.Columns.Add("Zone", "Zone");
            ServersGrid.Columns.Add("Host", "Host");
            ServersGrid.Columns.Add("Ping", "Ping");
            ServersGrid.Columns[0].AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill;
            ServersGrid.Columns[1].AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill;
            ServersGrid.Columns[PingColumnIndex].Width = 120;
            ServersGrid.AllowUserToOrderColumns = false;
            foreach (DataGridViewColumn col in ServersGrid.Columns)
            {
                col.SortMode = DataGridViewColumnSortMode.NotSortable;
            }

            foreach (var region in RegionCatalog.All)
            {
                int rowIndex = ServersGrid.Rows.Add(region.Name, region.Host, "ping");
                ServersGrid.Rows[rowIndex].Tag = region;
                regionRows[region.Id] = rowIndex;
            }
        }

// Определение применённой зоны и отметка её в таблице
        private async Task RefreshAppliedZoneAsync()
        {
            var (_, ruleRegionId) = await FirewallService.GetAppliedRuleAsync(SelectedPlatform);
            string? zoneId = HostsFileService.GetAppliedRegionId() ?? ruleRegionId;
            appliedZoneId = zoneId is not null && RegionCatalog.Find(zoneId) is not null ? zoneId : null;

            foreach (var region in RegionCatalog.All)
            {
                DataGridViewRow row = ServersGrid.Rows[regionRows[region.Id]];
                bool applied = region.Id == appliedZoneId;
                row.Cells[ZoneColumnIndex].Value = (applied ? "✓ " : "") + region.Name;
                row.Cells[ZoneColumnIndex].Style.ForeColor = applied ? Themes.Success : Themes.Text;
                row.Cells[ZoneColumnIndex].Style.SelectionForeColor = Themes.Text;
                row.Cells[HostColumnIndex].Style.ForeColor = applied ? Themes.Success : Themes.Text;
                row.Cells[HostColumnIndex].Style.SelectionForeColor = Themes.Text;
            }
        }

        private void PingTimer_Tick(object? sender, EventArgs e)
        {
            UpdatePings();
        }

        // Получение задержки до серверов Amazon GameLift
        private void UpdatePings()
        {
            pingCycle++;
            pendingPings = RegionCatalog.All.Count;
            foreach (var region in RegionCatalog.All)
            {
                _ = PingAsync(region.Host, regionRows[region.Id], pingCycle);
            }
        }

        private async Task PingAsync(string host, int rowIndex, int cycleId)
        {
            DataGridViewRow row = ServersGrid.Rows[rowIndex];
            try
            {
                using var ping = new Ping();
                PingReply reply = await ping.SendPingAsync(host);
                if (reply.Status == IPStatus.Success)
                {
                    pings[rowIndex] = reply.RoundtripTime;
                    Color pingColor = reply.RoundtripTime < 100 ? Themes.Success :
                                      reply.RoundtripTime < 200 ? Themes.Warning : Themes.Danger;
                    row.Cells[PingColumnIndex].Value = $"{reply.RoundtripTime} ms";
                    row.Cells[PingColumnIndex].Style.ForeColor = pingColor;
                    row.Cells[PingColumnIndex].Style.SelectionForeColor = pingColor;
                }
                else
                {
                    pings[rowIndex] = -1;
                    row.Cells[PingColumnIndex].Value = "Error";
                    row.Cells[PingColumnIndex].Style.ForeColor = Themes.Danger;
                    row.Cells[PingColumnIndex].Style.SelectionForeColor = Themes.Danger;
                }
            }
            catch
            {
                pings[rowIndex] = -1;
                row.Cells[PingColumnIndex].Value = "Error";
                row.Cells[PingColumnIndex].Style.ForeColor = Themes.Danger;
                row.Cells[PingColumnIndex].Style.SelectionForeColor = Themes.Danger;
            }

            if (cycleId == pingCycle && Interlocked.Decrement(ref pendingPings) == 0)
            {
                SelectBestPingRow();
            }
        }

        // Выделение строки с минимальной задержкой
        private void SelectBestPingRow()
        {
            int bestRow = -1;
            long bestMs = long.MaxValue;
            foreach (var kv in pings)
            {
                if (kv.Value >= 0 && kv.Value < bestMs)
                {
                    bestMs = kv.Value;
                    bestRow = kv.Key;
                }
            }

            if (bestRow >= 0)
            {
                ServersGrid.CurrentCell = ServersGrid.Rows[bestRow].Cells[ZoneColumnIndex];
                ServersGrid.Rows[bestRow].Selected = true;
            }
        }

        // Выбор файла исполняемого файла игры
        private void BtnSelectFile_Click(object? sender, EventArgs e)
        {
            openFileDialog.Title = "Select game executable";
            openFileDialog.Filter = "Dead by Daylight executables|" +
                "DeadByDaylight-Win64-Shipping.exe;DeadByDaylight-EGS-Shipping.exe;DeadByDaylight-WinGDK-Shipping.exe|" +
                "All files|*.*";
            if (openFileDialog.ShowDialog() == DialogResult.OK)
            {
                string? platform = DetectPlatform(openFileDialog.FileName);
                if (platform is null)
                {
                    MessageBox.Show("Unrecognized executable file");
                    return;
                }
                detectedPlatform = platform;
                FilePath.Text = openFileDialog.FileName;
                PathLabel.Text = $"Path to {PlatformExecutable[platform]}";
            }
        }

        private static string? DetectPlatform(string filePath)
        {
            string name = Path.GetFileName(filePath);
            foreach (var pair in PlatformExecutable)
            {
                if (string.Equals(name, pair.Value, StringComparison.OrdinalIgnoreCase))
                {
                    return pair.Key;
                }
            }
            return null;
        }

        // Создание правил брандмауэра для выбранного региона
        private async void BtnCreateRules_Click(object? sender, EventArgs e)
        {
            if (!ValidateSelection(out var region))
            {
                return;
            }

            SetBusy(true);
            try
            {
                OpResult hosts = HostsFileService.Apply(region);
                if (!hosts.Success)
                {
                    MessageBox.Show(hosts.Message);
                    return;
                }

                OpResult firewall = await FirewallService.CreateRulesAsync(region, FilePath.Text, SelectedPlatform);
                if (firewall.Success)
                {
                    DnsUtil.Flush();
                    MessageBox.Show("Rules have been successfully added");
                }
                else
                {
                    MessageBox.Show(firewall.Message);
                }

                await RefreshAppliedZoneAsync();
                UpdatePings();
            }
            finally
            {
                SetBusy(false);
            }
        }

        // Удаление правил брандмауэра для выбранного региона
        private async void BtnRemoveRules_Click(object? sender, EventArgs e)
        {
            if (SelectedRegion is null)
            {
                MessageBox.Show("Please select the region to be deleted");
                return;
            }

            SetBusy(true);
            try
            {
                OpResult hosts = HostsFileService.Clear();
                if (!hosts.Success)
                {
                    MessageBox.Show(hosts.Message);
                    return;
                }

                OpResult firewall = await FirewallService.RemoveRulesAsync(SelectedPlatform);
                if (firewall.Success)
                {
                    DnsUtil.Flush();
                    MessageBox.Show("Rules have been successfully removed");
                }
                else
                {
                    MessageBox.Show(firewall.Message);
                }

                await RefreshAppliedZoneAsync();
                UpdatePings();
            }
            finally
            {
                SetBusy(false);
            }
        }

        private void GithubLink_Click(object? sender, EventArgs e)
        {
            try
            {
                Process.Start(new ProcessStartInfo("https://github.com/rezzocrypt/Dead-by-Daylight-Server-Fix") { UseShellExecute = true });
            }
            catch
            {
            }
        }

        private bool ValidateSelection(out GameliftRegion region)
        {
            region = SelectedRegion!;
            if (region is null)
            {
                MessageBox.Show("Please select a region");
                return false;
            }
            if (FilePath.Text.Length == 0)
            {
                MessageBox.Show("Please specify the path to the executable file");
                return false;
            }
            return true;
        }

        private void SetBusy(bool busy)
        {
            btnCreateRules.Enabled = !busy;
            btnRemoveRules.Enabled = !busy;
            btnSelectFile.Enabled = !busy;
        }
    }
}