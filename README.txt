■これは何?

XM6iをNetBSD上で動かす場合、 ROM30.DAT がないと実機と同じ設定では
SCSIからの起動ができず Pluto-X の機能を使う必要がありますが、
このツールで生成されたダミーの ROM30.DAT を使うと実機と同じ起動設定で
一応SCSIからも NetBSD/x68k の起動ができるようになります。
Human68k でもテストしたところこちらもSCSI起動可能でした。

また、ついでの機能として Windows用の XM6Util と同じく Human68kの
フロッピーイメージ(および XM6Util.exe のバイナリ)から
SCSIINROM.DAT と SCSIEXROM.DAT 相当のROMイメージを生成することも
できます。

■使い方

NetBSD上で Makefile と mkscsirom.c を置いたディレクトリで make すると
mkscsiin mkscsiex mkrom30
の3つのバイナリができます。

XM6Util.exe のバイナリと HUMAN302.XDF のイメージを用意して
それぞれのコマンドで
% mkscsiin XM6Util.exe HUMAN302.XDF SCSIINROM.ROM
% mkscsiex XM6Util.exe HUMAN302.XDF SCSIEXROM.ROM
% mkrom30  XM6Util.exe HUMAN302.XDF ROM30.ROM
のようにすればそれぞれダミーのROMが作成されます。
出力のファイル名は任意です。あくまでもダミーなので XM6i の起動時には
中身が異なるという警告が出ますが起動自体は動きます。

XM6Util.exe や Human68k (および他のROMイメージ) の取得については
NetBSD/x68k on XM6i のページ
http://www.ceres.dti.ne.jp/tsutsui/netbsd/x68k/NetBSD-x68k-on-XM6i.html
を参照してください。

■注意点

入力の XM6Util.exe と HUMAN302.XDF についてはファイルサイズしか
正当性のチェックしていないので、壊れたファイルを指定すると壊れた
ROMイメージができてしまいます。

XM6iのバイナリはNetBSD(とMacOS)しかないのでNetBSD以外でビルドする意味は
あまりないと思いますが、 be32dec() と be32inc() の関数がない場合は
ソース中にある関数を使うように書き換えれば動くかもしれません(未テスト)。

ダミーの ROM30.DAT と言ってもSCSI起動以外の内容は一切入っていないので
それらの機能は使用できません(NetBSD/x68kではそのへん関係ありませんが)。

■SCSI起動について

ROM30.DATのうち、実際にSCSI起動に必要なのは 0xfc0000～0xfc001fまでの
SCSI ID 0～7 に対応する SCSI BOOT のベクタ(SRAMで指定するROMアドレス)
と 0xfc0024 にある ”SCSIIN” の文字列だけで、実際のSCSIアクセスを行う
SCSI IOCSルーチンおよび起動ルーチンは IPLROM30.DAT の中にあります。

よって、SCSI起動をさせるだけならこんなツールを使わずとも上記の
ベクタアドレスと SCSIIN の文字列だけ用意すればいいのですが、
せっかくだから XM6Util.exe の自前の起動プログラムを調べてみるのも
おもしろいかなといろいろいじっていたら一応起動できるようになったので
ツールとして書き直してみたのでした。ツールとして公開したというよりは、
XM6Util がどういうことをしているのか、X68030でのSCSI起動には何が必要か、
の調査結果をコードでしたためたというのが実態なので、コードや使い勝手が
アレなのは目をつぶって、実際に何をやっているのかという観点でソースを
読んでみてください。

X680x0のブートストラップについては Togetter まとめ
http://togetter.com/li/410617
にもありますのでそちらも参照してください。

---
Izumi Tsutsui
tsutsui@ceres.dti.ne.jp