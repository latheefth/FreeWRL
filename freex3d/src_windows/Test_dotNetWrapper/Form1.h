#pragma once


namespace Test_dotNetWrapper {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace libFreeWRL_dotNetwrapper;

	/// <summary>
	/// Summary for Form1
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class freewrlUC : public System::Windows::Forms::Form
	{
	public:
        FreewrlLib ^freewrllib;
		freewrlUC(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
            //freewrllib = gcnew FreewrlLib();

		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~freewrlUC()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Label^  lblStatus;
	private: System::Windows::Forms::Timer^  timer1;
	private: System::Windows::Forms::Label^  lblStatus2;
	private: System::Windows::Forms::Button^  button1;
	private: System::Windows::Forms::Panel^  panel1;

	private: System::ComponentModel::IContainer^  components;
	protected: 

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>


#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			this->lblStatus = (gcnew System::Windows::Forms::Label());
			this->timer1 = (gcnew System::Windows::Forms::Timer(this->components));
			this->lblStatus2 = (gcnew System::Windows::Forms::Label());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->panel1 = (gcnew System::Windows::Forms::Panel());
			this->SuspendLayout();
			// 
			// lblStatus
			// 
			this->lblStatus->AutoSize = true;
			this->lblStatus->Location = System::Drawing::Point(83, 2);
			this->lblStatus->Name = L"lblStatus";
			this->lblStatus->Size = System::Drawing::Size(35, 13);
			this->lblStatus->TabIndex = 0;
			this->lblStatus->Text = L"label1";
			// 
			// timer1
			// 
			this->timer1->Enabled = true;
			this->timer1->Interval = 50;
			this->timer1->Tick += gcnew System::EventHandler(this, &freewrlUC::timer1_Tick);
			// 
			// lblStatus2
			// 
			this->lblStatus2->AutoSize = true;
			this->lblStatus2->Location = System::Drawing::Point(213, 2);
			this->lblStatus2->Name = L"lblStatus2";
			this->lblStatus2->Size = System::Drawing::Size(35, 13);
			this->lblStatus2->TabIndex = 1;
			this->lblStatus2->Text = L"label1";
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(2, 2);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(75, 23);
			this->button1->TabIndex = 2;
			this->button1->Text = L"LoadScene";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &freewrlUC::button1_Click);
			// 
			// panel1
			// 
			this->panel1->Dock = System::Windows::Forms::DockStyle::Bottom;
			this->panel1->Location = System::Drawing::Point(0, 32);
			this->panel1->Name = L"panel1";
			this->panel1->Size = System::Drawing::Size(412, 298);
			this->panel1->TabIndex = 3;
			this->panel1->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &freewrlUC::panel1_MouseDown);
			this->panel1->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &freewrlUC::panel1_MouseMove);
			this->panel1->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &freewrlUC::panel1_MouseUp);
			// 
			// freewrlUC
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(412, 330);
			this->Controls->Add(this->panel1);
			this->Controls->Add(this->button1);
			this->Controls->Add(this->lblStatus2);
			this->Controls->Add(this->lblStatus);
			this->Name = L"freewrlUC";
			this->Text = L"Form1";
			this->Load += gcnew System::EventHandler(this, &freewrlUC::freewrlUC_Load);
			this->Shown += gcnew System::EventHandler(this, &freewrlUC::freewrlUC_Shown);
			this->ClientSizeChanged += gcnew System::EventHandler(this, &freewrlUC::freewrlUC_ClientSizeChanged);
			this->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &freewrlUC::freewrlUC_KeyDown);
			this->KeyPress += gcnew System::Windows::Forms::KeyPressEventHandler(this, &freewrlUC::freewrlUC_KeyPress);
			this->KeyUp += gcnew System::Windows::Forms::KeyEventHandler(this, &freewrlUC::freewrlUC_KeyUp);
			this->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &freewrlUC::freewrlUC_MouseDown);
			this->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &freewrlUC::freewrlUC_MouseMove);
			this->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &freewrlUC::freewrlUC_MouseUp);
			this->Resize += gcnew System::EventHandler(this, &freewrlUC::freewrlUC_Resize);
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion
	private: FreewrlLib::MouseButton convertButton(System::Windows::Forms::MouseButtons mb)
        {
			FreewrlLib::MouseButton b;
            switch (mb)
            {
			case System::Windows::Forms::MouseButtons::Left:
				b = FreewrlLib::MouseButton::LEFT; break;
			case System::Windows::Forms::MouseButtons::Middle:
					b = FreewrlLib::MouseButton::MIDDLE; break;
			case System::Windows::Forms::MouseButtons::Right:
					b = FreewrlLib::MouseButton::RIGHT; break;
                default:
					b = FreewrlLib::MouseButton::NONE; break;
            }
            return b;
        }

	private: System::Void freewrlUC_Load(System::Object^  sender, System::EventArgs^  e) {
            //freewrllib = gcnew FreewrlLib(this->Width, this->Height, this->Handle, false);

			 }
	private: System::Void freewrlUC_Resize(System::Object^  sender, System::EventArgs^  e) {
            freewrllib->onResize( panel1->Width,panel1->Height);
			 }
	private: System::Void freewrlUC_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e) {
            lblStatus->Text = e->KeyValue.ToString();
			freewrllib->onKey(FreewrlLib::KeyAction::KEYDOWN, e->KeyValue);

			 }
	private: System::Void freewrlUC_KeyPress(System::Object^  sender, System::Windows::Forms::KeyPressEventArgs^  e) {
            lblStatus->Text = e->KeyChar.ToString();
			freewrllib->onKey(FreewrlLib::KeyAction::KEYPRESS, Convert::ToInt32(e->KeyChar));

			 }
	private: System::Void freewrlUC_KeyUp(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e) {
            lblStatus->Text = e->KeyValue.ToString();
			freewrllib->onKey(FreewrlLib::KeyAction::KEYUP, e->KeyValue);

			 }
	private: System::Void setCursorStyle(int style){
		switch(style){
			case 0: panel1->Cursor = Cursors::Arrow; break;
			case 1:	panel1->Cursor = Cursors::Hand; break;
			case 2: panel1->Cursor = Cursors::Cross; break;
			default: panel1->Cursor = Cursors::Arrow;
		}
	}
	private: System::Void freewrlUC_MouseDown(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
			 }
	private: System::Void freewrlUC_MouseMove(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
			 }
	private: System::Void freewrlUC_MouseUp(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
			 }
	private: System::Void timer1_Tick(System::Object^  sender, System::EventArgs^  e) {
            timer1->Stop();
			freewrllib->onTick(timer1->Interval);
            lblStatus->Text += ".";
            timer1->Start();
			lblStatus2->Text = freewrllib->message;

			 }
private: System::Void freewrlUC_Shown(System::Object^  sender, System::EventArgs^  e) {
				 IntPtr handle = panel1->Handle; //The value of the Handle property is a Windows HWND
            lblStatus->Text = handle.ToString() + " " + panel1->Size.Height.ToString() + " " + panel1->Size.Width.ToString();
            //freewrllib->onInit(handle, this->ClientSize.Width,this->ClientSize.Height); //this->Size.Width,this->Size.Height);
            freewrllib = gcnew FreewrlLib(panel1->Width, panel1->Height, panel1->Handle, false);
			//freewrllib->onLoad("E:/source2/tests/2.x3d");

		 }

private: System::Void freewrlUC_ClientSizeChanged(System::Object^  sender, System::EventArgs^  e) {
            freewrllib->onResize( panel1->ClientSize.Width,panel1->ClientSize.Height);
		 }
private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {
			freewrllib->onLoad("C:/Users/Public/dev/source2/tests/sensors/DragCascade.x3d");

		 }
private: System::Void panel1_MouseDown(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
			int cursorStyle;
            lblStatus->Text = e->Location.ToString() + e->Button.ToString();
			cursorStyle = freewrllib->onMouse(FreewrlLib::MouseAction::MOUSEDOWN, convertButton(e->Button), e->X, e->Y);
			lblStatus2->Text = freewrllib->message;
			setCursorStyle(cursorStyle);
}
private: System::Void panel1_MouseMove(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
			int cursorStyle;
            lblStatus->Text = e->Location.ToString() + e->Button.ToString();
			cursorStyle = freewrllib->onMouse(FreewrlLib::MouseAction::MOUSEMOVE, convertButton(e->Button), e->X, e->Y);
			setCursorStyle(cursorStyle);
}
private: System::Void panel1_MouseUp(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
			int cursorStyle;
            lblStatus->Text = e->Location.ToString() + e->Button.ToString();
			cursorStyle = freewrllib->onMouse(FreewrlLib::MouseAction::MOUSEUP, convertButton(e->Button), e->X, e->Y);
			setCursorStyle(cursorStyle);
}
};
}

