![Peryan Logo](./peryan.png)

## Peryanについて

Peryanによって書かれたエラトステネスのふるい(1000より小さい素数を列挙)

    var length = 1000
    
    var table = [Bool](length, true)
    
    table[0] = false
    table[1] = false
    
    repeat length
    	if table[cnt] == false : continue
    
    	var cnt_ = cnt
    	var cur = cnt_ + cnt_
    
    	repeat
    		cur += cnt_
    		if !(cur < length) : break
    
    		table[cur] = false
    	loop
    loop
    
    repeat length
    	if table[cnt] : mes String(cnt)
    loop

Peryanは以下のような目標を持つプログラミング言語です。

* 強い静的型付けとLLVMを用いた静的コンパイル
* オブジェクト指向を含む現代的な言語機能
* HSPとの互換性

Peryanは、これらの目標を達成することにより、高い実用性を持った言語となる事を目指しています。

## 処理系のコンパイル

現在、PeryanはPOSIX環境とWindowsをサポートしており、それぞれMac OS XとWindows 8の環境で開発されています。

処理系はSTLのみを利用するC++で書かれており、コンパイルにはLLVM 3.8が必要です。
Linuxでは各ディストリビューションのパッケージマネージャ、OSXではbrew install llvmで事前にLLVMをインストールする必要があります。

    make
    make test

Windowsでは現状Visual C++ 2012によるコンパイルのみサポートしています。詳しくはbuild/win32を御覧ください。

## PeryanによるPeryanプログラムのコンパイル (POSIX)

    cd build/unix
    make
    ./bin/peryan --runtime-path . ../../test/integration/cases/Sieve.pr -o sieve
    ./sieve

## 文法概観

### 関数定義

    func thisIsFunction (arg1 :: Int, arg2 :: Double, arg3 :: String) :: Int {
    	thisIsStatement
    	return 0
    }

ただし型推論により以下のように型宣言を省略することもできます。

    func myAbs (x) {
    	if x > 0 {
    		return x
    	} else {
    		return -x
    	}
    }
    mes String(myAbs(-1234))

### 変数定義

    var thisIsVariable :: Int = 123

同様に、型推論の有効な範囲内で型宣言を省略することができます。

    var thisIsVariable = 123

## 今後の展望

Peryanはオブジェクト指向などをはじめとして主要機能の多くが未だ実装されておらず、開発の最初期段階にあります。

今後実装される予定の物には以下のものがあります。
また、詳細なドキュメントも今後用意される予定です。

* オブジェクト指向
* variant型
* パターンマッチ
* クロージャ
* 部分適用
* ガード
* 各種HSP互換機能
* ランタイムライブラリ
* コルーチン

## 処理系のコードについて

処理系のコードはメモリをまともに解放しない、頻繁にSegmentation Faultで落ちるなど、ひどい出来です。
アドバイス等ございましたら、@peryaudoまでお伝え頂けると非常に助かります。

## ライセンス

Peryanの処理系はMITライセンスの下に提供されます。詳しくはLICENSEを御覧ください。
また、test/unit/gtest/下のGoogle Testは新BSDライセンスの下に提供されます。詳しくはtest/unit/gtest/COPYINGを御覧ください。

## 参考文献

* 柏木 餅子, 風薬 『きつねさんでもわかるLLVM　〜コンパイラを自作するためのガイドブック〜』 2013年
* Terence Parr 『言語実装パターン』 2011年
* Benjamin C. Pierce 『型システム入門 －プログラミング言語と型の理論－』2013年
* Benjamin C. Pierce, David N. Turner "Local Type Inference" ACM TOPLAS Vol. 22 No.1 Jan. 2000


