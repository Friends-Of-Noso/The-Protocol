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
;

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
  ticks: Int64;
begin
  WriteLn('Beginning of thread');
  ticks:= GetTickCount64;
  while (not Terminated) do
  begin
    if (GetTickCount64 - ticks) > 10000 then
    begin
      WriteLn('About 10s have past');
      ticks:= GetTickCount64;
    end;
    Sleep(1);
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
  thread:= TCommThread.Create(True);
  try
    WriteLn('Starting thread');
    thread.Start;
    WriteLn('*********************');
    WriteLn('* Hit Enter to exit *');
    WriteLn('*********************');
    ReadLn;
  finally
    WriteLn('Terminating Thread');
    thread.Terminate;
    WriteLn('Waiting for thread shutdown');
    thread.WaitFor;
  end;
end.
