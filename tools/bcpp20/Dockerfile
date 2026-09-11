# tools/bcpp20 用的工具映像：解磁碟映像與 ZIP，也用來執行 dosgolem 編出的靜態執行檔。
# 基底釘 digest：同一份 Dockerfile 在不同日子建出來的 unzip／7z 版本要一致，
# 否則「某個 implode 檔解不開」會變成隨建置日期漂移的問題。
FROM python:3.13-alpine@sha256:540c7d91f98ff6880174c40e99067bf5941eb54d818a7a5e094d188b196a934d

# unzip：Info-ZIP，支援 PKZIP 1.x 的 shrink（method 1）與 implode（method 6）；
#        busybox 內建的 unzip 不支援，Borland 磁片裡的內層 ZIP 全是這兩種。
# 7zip：解 WinWorld 等來源常見的 .7z 封存。
# mtools：交叉驗證 fat12.py 讀 FAT12 映像的結果。
RUN apk add --no-cache unzip=6.0-r16 7zip=26.01-r0 mtools=4.0.49-r0 \
 && unzip -v | head -1 && 7z | head -2 && mdir --version | head -1
