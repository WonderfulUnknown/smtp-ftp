#include "stdafx.h"
#include "MySocket.h"
#include "smtp_server.h"
#include "smtp_serverDlg.h"
#include "base.h"

MySocket::MySocket()
{
	step = 1;
	IsData = false;
	Quit = false;
}


MySocket::~MySocket()
{
}


//����nErrorCode,��������ʱ��MFC����ṩ�������׽������µ�״̬
//�����0����������ִ�У�û�д��󣻷�0���׽��ֶ������
void MySocket::OnAccept(int nErrorCode)
{
	// TODO: �ڴ�����ר�ô����/����û���
	CString log;
	MySocket *sock = new MySocket();

	AfxGetMainWnd()->GetDlgItemText(IDC_Log, log);

	if (Accept(*sock))
	{
		msg = "220 smtp 127.0.0.1 is ready\r\n";
		sock->Send(msg, strlen(msg), 0);
		//����FD_READ�¼�������OnReceive����
		sock->AsyncSelect(FD_READ);

		log = "TCP���ӳɹ�\r\n";
	}
	else
	{
		log = "TCP����ʧ��\r\n";
		sock->Close();
	}
	AfxGetMainWnd()->SetDlgItemText(IDC_Log, log);

	CAsyncSocket::OnAccept(nErrorCode);
}


void MySocket::OnClose(int nErrorCode)
{
	// TODO: �ڴ�����ר�ô����/����û���

	CAsyncSocket::OnClose(nErrorCode);
}


void MySocket::OnReceive(int nErrorCode)
{
	// TODO: �ڴ�����ר�ô����/����û���
	
	//ÿ��receive֮ǰ��Ҫ�ѻ���������
	memset(data, 0, sizeof(data));  
	//length�洢�����յ���Ϣ�ĳ��ȣ����յ������ݴ浽data��
	length = Receive(data, sizeof(data), 0);

	//�����ݴӵȴ����ж��뻺���������Ҳ������ݴӻ��������
	//length = Receive(data, sizeof(data), MSG_PEEK);
	//�����������
	//length = Receive(data, sizeof(data), MSG_OOB);

	receive = data;
	CString log;
	//����һ�ε�logд��
	AfxGetMainWnd()->GetDlgItemText(IDC_Log, log);
	if (!IsData)
		log = log + L"C:" + receive.Left(length);
	else
		log = log + L"C:" + L"data\r\n";

	if (length != SOCKET_ERROR)
	{
		if (!IsData)//���յ�����
		{
			//�����û���������ݽ���Ӧ��
			if ((receive.Left(4) == "HELO" || receive.Left(4) == "EHLO") && step == 1)
				msg = "250 OK hello smtp 127.0.0.1\r\n";
			else if (receive.Left(10) == "MAIL FROM:" && step == 2)
				msg = "250 Sender OK\r\n";
			else if (receive.Left(8) == "RCPT TO:" && step == 3)
				msg = "250 Receiver OK\r\n";
			else if (receive.Left(4) == "DATA" && step == 4)
			{
				IsData = true;//����յ�DATA���˵���������Ľ��յ���������
				msg = "354 Start mail input,end with <CRLF>.<CRLF>\r\n";
			}
			else if (receive.Left(4) == "QUIT")
			{
				msg = "221 smtp 127.0.0.1 server closing connection\r\n";
				Quit = true;//�ͻ����˳������ֹ����
			}
			else
				msg = "550 Error: bad syntax\r\n";
	
			Send(msg, strlen(msg), 0);//����Ӧ��

			step++;
			log = log + L"S:" + (CString)msg;
			AsyncSelect(FD_READ);//�������պ���
			AfxGetMainWnd()->SetDlgItemText(IDC_Log, log);//д������־
			return;
		}
		//���������ݲ�Ӧ��
		else
		{
			//��ȡ�ʼ�����������
			CString str = receive.Left(length);
			AfxGetMainWnd()->SetDlgItemText(IDC_INFO, str);

			//<CRLF>.<CRLF>
			if (receive.Find(L"\r\n.\r\n") != -1)//���ݽ������
			{
				IsData = false;
				
				msg = "250 Message accepted\r\n";//����Ӧ��
				Send(msg, strlen(msg), 0);
				
				log = log + "S:" + (CString)msg;
				AfxGetMainWnd()->SetDlgItemText(IDC_Log, log);
				
				//AfxGetMainWnd()->GetDlgItemText(IDC_INFO, pic);
				
				if (pic.Find(L"Content-Type: image/bmp") != -1)//��������bmpͼƬ
				{
					//��ȡbmpͼƬ��base64����
					int Attachment_Start = pic.Find(L"Content-Disposition: attachment", 0);
					int Bmp_Start = pic.Find(L"\r\n\r\n", Attachment_Start);
					CString Start = pic.Mid(Bmp_Start + 4, pic.GetLength() - Bmp_Start - 4);
					int length = Start.Find(L"\r\n\r\n", 0);
					pic = Start.Left(length);
					HBITMAP picture;
					//����
					DeCode(pic, picture);
					//���뵽�Ի���
					Csmtp_serverDlg *CurrentApp = (Csmtp_serverDlg *)AfxGetApp();
					Csmtp_serverDlg *CurrentDlg = (Csmtp_serverDlg *)CurrentApp->m_hWnd;
					//CurrentDlg->m_Bmp.SetBitmap(picture);

				}
			}
			AsyncSelect(FD_READ);

			return;
		}
	}
	if (Quit)//�˳�
	{
		MySocket sock;
		sock.OnClose(0);
		Quit = false;
		return;
	}
	
	CAsyncSocket::OnReceive(nErrorCode);
}