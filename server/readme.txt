﻿MikuMikuOnline server 0.1.7

◆サーバーの起動方法

Server.exeを実行します。
外部からの接続を受け付けるにはポートの開放が必要になります。

◆ポートについて

TCPポート39390, UDPポート39390を使用します。


◆サーバーの設定

config.jsonをテキストエディタで編集することで、サーバーの設定を変更することができます。

[port]
	待ち受けポートの番号です。通常は39390のままにしてください。

[server_name]
	サーバーの名前です。自由に設定できます。
	
[stage]
	サーバーが使用するステージ名です。
	指定したステージを持っていないクライアントは接続を拒否されます。
	
[capacity]
	サーバーの最大同時接続数です。
	
	
[receive_limit_1]
	平均受信量制限です。単位は byte/sです。
	クライアントからの平均受信量がこの数値を超えている間、
	そのクライアントからの受信を無視します。
	
[receive_limit_2]
	平均受信量制限です。単位は byte/sです。
	クライアントからの平均受信量がこの数値を超えた瞬間に、
	そのクライアントとのセッションを強制的に切断します。
	
	
[blocking_address_patterns]
	接続を拒否するIPアドレスのリストです。ワイルドカードを使用できます。
	

--

mmo@h2so5.net
http://mmo.h2so5.net/