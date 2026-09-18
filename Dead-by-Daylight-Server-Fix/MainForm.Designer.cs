namespace SelectRegionForDbd
{
    partial class MainForm
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent() {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
            openFileDialog = new OpenFileDialog();
            lblTitle = new Label();
            lblSubtitle = new Label();
            ServersGrid = new DataGridView();
            PathLabel = new Label();
            FilePath = new TextBox();
            btnSelectFile = new ModernButton();
            btnRemoveRules = new ModernButton();
            btnCreateRules = new ModernButton();
            statusStrip = new StatusStrip();
            githubLink = new ToolStripStatusLabel();
            versionLabel = new ToolStripStatusLabel();
            ((System.ComponentModel.ISupportInitialize)ServersGrid).BeginInit();
            statusStrip.SuspendLayout();
            SuspendLayout();
            // 
            // lblTitle
            // 
            lblTitle.AutoSize = true;
            lblTitle.Font = new Font("Segoe UI", 19F, FontStyle.Bold, GraphicsUnit.Point, 204);
            lblTitle.ForeColor = Color.White;
            lblTitle.Location = new Point(12, 9);
            lblTitle.Name = "lblTitle";
            lblTitle.Size = new Size(225, 36);
            lblTitle.TabIndex = 41;
            lblTitle.Text = "Dead by Daylight";
            // 
            // lblSubtitle
            // 
            lblSubtitle.AutoSize = true;
            lblSubtitle.Font = new Font("Segoe UI", 9.5F, FontStyle.Regular, GraphicsUnit.Point, 204);
            lblSubtitle.ForeColor = Color.FromArgb(152, 162, 179);
            lblSubtitle.Location = new Point(24, 45);
            lblSubtitle.Name = "lblSubtitle";
            lblSubtitle.Size = new Size(96, 17);
            lblSubtitle.TabIndex = 42;
            lblSubtitle.Text = "region selector";
            // 
            // ServersGrid
            // 
            ServersGrid.AllowUserToAddRows = false;
            ServersGrid.AllowUserToDeleteRows = false;
            ServersGrid.AllowUserToResizeRows = false;
            ServersGrid.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            ServersGrid.ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode.DisableResizing;
            ServersGrid.Font = new Font("Segoe UI", 9F, FontStyle.Regular, GraphicsUnit.Point, 204);
            ServersGrid.Location = new Point(24, 82);
            ServersGrid.MultiSelect = false;
            ServersGrid.Name = "ServersGrid";
            ServersGrid.ReadOnly = true;
            ServersGrid.RowHeadersVisible = false;
            ServersGrid.RowTemplate.Height = 22;
            ServersGrid.SelectionMode = DataGridViewSelectionMode.FullRowSelect;
            ServersGrid.Size = new Size(712, 372);
            ServersGrid.TabIndex = 40;
            // 
            // PathLabel
            // 
            PathLabel.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
            PathLabel.AutoSize = true;
            PathLabel.Font = new Font("Segoe UI", 9F, FontStyle.Regular, GraphicsUnit.Point, 204);
            PathLabel.ForeColor = Color.FromArgb(152, 162, 179);
            PathLabel.Location = new Point(24, 470);
            PathLabel.Name = "PathLabel";
            PathLabel.Size = new Size(187, 15);
            PathLabel.TabIndex = 28;
            PathLabel.Text = "Path to DeadByDaylight binary file";
            // 
            // FilePath
            // 
            FilePath.Anchor = AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            FilePath.BorderStyle = BorderStyle.FixedSingle;
            FilePath.Font = new Font("Segoe UI", 10F, FontStyle.Regular, GraphicsUnit.Point, 204);
            FilePath.Location = new Point(24, 490);
            FilePath.Name = "FilePath";
            FilePath.ReadOnly = true;
            FilePath.Size = new Size(550, 25);
            FilePath.TabIndex = 29;
            FilePath.TabStop = false;
            // 
            // btnSelectFile
            // 
            btnSelectFile.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnSelectFile.BackColor = Color.FromArgb(17, 21, 28);
            btnSelectFile.FlatStyle = FlatStyle.Flat;
            btnSelectFile.Font = new Font("Microsoft Sans Serif", 10F, FontStyle.Bold);
            btnSelectFile.Location = new Point(584, 490);
            btnSelectFile.Name = "btnSelectFile";
            btnSelectFile.Size = new Size(152, 25);
            btnSelectFile.TabIndex = 30;
            btnSelectFile.Text = "Browse";
            btnSelectFile.UseVisualStyleBackColor = false;
            btnSelectFile.Click += BtnSelectFile_Click;
            // 
            // btnRemoveRules
            // 
            btnRemoveRules.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnRemoveRules.BackColor = Color.FromArgb(17, 21, 28);
            btnRemoveRules.ButtonStyle = ModernButtonStyle.Danger;
            btnRemoveRules.FlatStyle = FlatStyle.Flat;
            btnRemoveRules.Font = new Font("Microsoft Sans Serif", 10F, FontStyle.Bold);
            btnRemoveRules.Location = new Point(422, 548);
            btnRemoveRules.Name = "btnRemoveRules";
            btnRemoveRules.Size = new Size(152, 39);
            btnRemoveRules.TabIndex = 33;
            btnRemoveRules.Text = "Remove Rules";
            btnRemoveRules.UseVisualStyleBackColor = false;
            btnRemoveRules.Click += BtnRemoveRules_Click;
            // 
            // btnCreateRules
            // 
            btnCreateRules.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            btnCreateRules.BackColor = Color.FromArgb(17, 21, 28);
            btnCreateRules.ButtonStyle = ModernButtonStyle.Primary;
            btnCreateRules.FlatStyle = FlatStyle.Flat;
            btnCreateRules.Font = new Font("Microsoft Sans Serif", 10F, FontStyle.Bold);
            btnCreateRules.Location = new Point(584, 548);
            btnCreateRules.Name = "btnCreateRules";
            btnCreateRules.Size = new Size(152, 39);
            btnCreateRules.TabIndex = 34;
            btnCreateRules.Text = "Create Rules";
            btnCreateRules.UseVisualStyleBackColor = false;
            btnCreateRules.Click += BtnCreateRules_Click;
            // 
            // statusStrip
            // 
            statusStrip.Items.AddRange(new ToolStripItem[] { githubLink, versionLabel });
            statusStrip.Location = new Point(0, 600);
            statusStrip.Name = "statusStrip";
            statusStrip.Size = new Size(760, 22);
            statusStrip.SizingGrip = false;
            statusStrip.TabIndex = 45;
            // 
            // githubLink
            // 
            githubLink.ForeColor = Color.FromArgb(127, 176, 255);
            githubLink.Name = "githubLink";
            githubLink.Size = new Size(708, 17);
            githubLink.Spring = true;
            githubLink.Text = "https://github.com/rezzocrypt/Dead-by-Daylight-Server-Fix";
            githubLink.ToolTipText = "Open repository in browser";
            githubLink.Click += GithubLink_Click;
            // 
            // versionLabel
            // 
            versionLabel.ForeColor = Color.FromArgb(152, 162, 179);
            versionLabel.Name = "versionLabel";
            versionLabel.Size = new Size(37, 17);
            versionLabel.Text = "v1.2.0";
            // 
            // MainForm
            // 
            AutoScaleDimensions = new SizeF(7F, 15F);
            AutoScaleMode = AutoScaleMode.Font;
            ClientSize = new Size(760, 622);
            Controls.Add(btnCreateRules);
            Controls.Add(btnRemoveRules);
            Controls.Add(btnSelectFile);
            Controls.Add(FilePath);
            Controls.Add(PathLabel);
            Controls.Add(ServersGrid);
            Controls.Add(lblSubtitle);
            Controls.Add(lblTitle);
            Controls.Add(statusStrip);
            Icon = (Icon)resources.GetObject("$this.Icon");
            MaximizeBox = false;
            MinimizeBox = false;
            Name = "MainForm";
            StartPosition = FormStartPosition.CenterScreen;
            Text = "Dead by Daylight Region Selector";
            ((System.ComponentModel.ISupportInitialize)ServersGrid).EndInit();
            statusStrip.ResumeLayout(false);
            statusStrip.PerformLayout();
            ResumeLayout(false);
            PerformLayout();
        }

        #endregion

        private OpenFileDialog openFileDialog;
        private Label lblTitle;
        private Label lblSubtitle;
        private DataGridView ServersGrid;
        private Label PathLabel;
        private TextBox FilePath;
private ModernButton btnSelectFile;
        private ModernButton btnRemoveRules;
        private ModernButton btnCreateRules;
        private StatusStrip statusStrip;
        private ToolStripStatusLabel githubLink;
        private ToolStripStatusLabel versionLabel;
    }
}