program PingPong;

{$mode ObjFPC}{$H+}

uses
{$IFDEF UNIX}
  cthreads
, cmem
,
{$IFEND}
  SysUtils
, Classes
, DateUtils
, SimpleSockets
;

const
  cServerIP = '127.0.0.1';
  cServerPort = 8080;
  cClientIP = '127.0.0.1';
  cLogFile = 'pingpong.log';

var
  isConnected: Boolean = False;

procedure DoLog(const AMsg: String);
var
  log: TextFile;
begin
  AssignFile(log, cLogFile);
  if FileExists(cLogFile) then
    Append(log)
  else
    Rewrite(log);
  WriteLn(log, Trim(AMsg));
  CloseFile(Log);
end;

type
{ TCommThread }
  TCommThread = class(TThread)
  private
  protected
    procedure Execute; override;
  public
    Constructor Create(CreateSuspended : boolean);
  end;

{ TCommThread }
procedure TCommThread.Execute;
var
  client: TSocket;
  Ping, Hello, Data: String;
  Timer: Integer;
  startTime: QWord;
begin
  WriteLn('Beginning of thread');
  Timer := 5000;
  client := TCPSocket(stIPv4);
  Hello:=
    'PSK ' +                               // Magic String
    cClientIP + ' ' +                      // Your IP
    '0.3.3Aa6 ' +                          // App version
    IntToStr(DateTimeToUnix(now, False)) + // Unix time. This need sto be in UTC, which it is not in this case
    #13#10
  ;
  Ping:=
    'PSK ' +                                      // Magic String
    '2 ' +                                        // Protocol version
    '0.3.3Aa6 ' +                                 // App version
    IntToStr(DateTimeToUnix(now, False)) + ' ' +  // Unix time. This need sto be in UTC, which it is not in this case
    '$PING ' +                                    // Magic string
    '0 ' +                                        // Current connections
    '0 ' +                                        // Block number
    '4E8A4743AA6083F3833DDA1216FE3717 ' +         // Block Hash (Genesis block hash)
    'D41D8CD98F00B204E9800998ECF8427E ' +         // Hash summary.psk (This is the MD5 hash for empty)
    '0 ' +                                        // Pending Orders
    'D41D8CD98F00B204E9800998ECF8427E ' +         // Hash blchhead.nos (This is the MD5 hash for empty)
    '0 ' +                                        // Connections status [0, 1,2,3]
    '8080 ' +                                     // Port
    'D41D8 ' +                                    // Hash(5) masternodes.txt (This is the MD5 hash for empty)
    '0 ' +                                        // MN Count
    '0000 ' +                                     // NMsData difference aka Best Hash?
    '0 ' +                                        // Checked Master Nodes
    'D41D8CD98F00B204E9800998ECF8427E ' +         // Hash gvts.psk (This is the MD5 hash for empty)
    'D41D8 ' +                                    // Hash(5) CFGs
    #13#10
  ;

  try
    WriteLn('Connecting');
    try
      Connect(client, cServerIP, cServerPort);
    except
      on E:Exception do
      begin
       WriteLn('Connect Error: ' + E.Message);
       isConnected:= False;
      end;
    end;
    isConnected:= True;
    //WriteLn('Sending Hello');
    SendStr(client, Hello);
    WriteLn('>>>>: ' + Trim(Hello));
    DoLog('>>>>: ' + Hello);
    //WriteLn('Hello sent');
    //WriteLn('Sending Ping');
    SendStr(Client, Ping);
    WriteLn('>>>>: ' + Trim(Ping));
    DoLog('>>>>: ' + Ping);
    //WriteLn('Ping sent');
    startTime := GetTickCount64;
    while not Terminated do
    begin
      if not DataAvailable(client, Timer) then
      begin
        if Terminated then break;
        //WriteLn('Sending Ping');
        SendStr(Client, Ping);
        WriteLn('>>>>: ' + Trim(Ping));
        DoLog('>>>>: ' + Ping);
        //WriteLn('Ping sent');
        startTime:=GetTickCount64;
        Timer := 5000;
      end
      else
       begin
         if Terminated then break;
        //WriteLn('Reading data');
        Data := ReceiveStr(client, 1024); // 1024 max length data can be shorter
        //WriteLn('Data read');
        WriteLn('<<<<: ' + Trim(Data));
        DoLog('<<<<: ' + Data);
        // Compute new wait period for ping to the next 5 second mark
        Timer := 5000 - (GetTickCount64 - startTime);
        if Timer < 0 then Timer := 0;
      end;
    end;
    WriteLn('Loop exited');
  finally
    WriteLn('Closing socket');
    CloseSocket(client);
    WriteLn('Socket closed');
    isConnected:= False;
  end;
end;

constructor TCommThread.Create(CreateSuspended: boolean);
begin
  inherited Create(CreateSuspended);
  FreeOnTerminate:= True;
end;

var
  thread: TCommThread = nil;

begin
  WriteLn('Ping Pong');
  if FileExists(cLogFile) then
    DeleteFile(cLogFile);
  thread:= TCommThread.Create(True);
  try
    WriteLn('Starting thread');
    thread.Start;
    WriteLn('*********************');
    WriteLn('* Hit Enter to exit *');
    WriteLn('*********************');
    ReadLn;
  finally
    if isConnected then
    begin
      WriteLn('Terminating Thread');
      thread.Terminate;
      WriteLn('Waiting for thread shutdown');
      thread.WaitFor;
    end;
  end;
end.
