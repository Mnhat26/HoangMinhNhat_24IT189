# MYLS – NetBSD ls Command Implementation

**Đề tài:** Xây dựng chương trình mô phỏng lệnh `ls(1)` trên hệ điều hành NetBSD.

**Sinh viên thực hiện:** Hoàng Minh Nhật

**Mã sinh viên:** 24IT189

**Hệ điều hành:** NetBSD 10.1 (amd64)

**Ngôn ngữ lập trình:** C (C99)

**Môn học:** Lập trình hệ thống

## 1. Giới thiệu

MYLS là chương trình dòng lệnh được viết bằng ngôn ngữ C, mô phỏng các chức năng của lệnh `ls` trên hệ điều hành UNIX/NetBSD.

Chương trình sử dụng các API hệ thống như `opendir()`, `readdir()`, `lstat()`, `getpwuid()`, `getgrgid()` và `readlink()` để đọc, xử lý và hiển thị thông tin tệp và thư mục.

Dự án được phát triển bằng Visual Studio Code trên Windows và biên dịch, kiểm thử trên NetBSD 10.1 thông qua SSH.

## 2. Các tùy chọn hỗ trợ

Chương trình đã xây dựng mã xử lý cho 19 tùy chọn:

| Option | Chức năng |
|---|---|
| `-A` | Hiển thị file ẩn, trừ `.` và `..` |
| `-a` | Hiển thị tất cả file |
| `-c` | Sử dụng thời gian thay đổi trạng thái file |
| `-d` | Hiển thị thông tin chính thư mục |
| `-F` | Phân loại tệp bằng ký hiệu `/`, `*`, `@` |
| `-f` | Không thực hiện sắp xếp |
| `-h` | Hiển thị kích thước dễ đọc |
| `-i` | Hiển thị số inode |
| `-k` | Hiển thị số block theo đơn vị 1 KiB |
| `-l` | Hiển thị thông tin chi tiết |
| `-n` | Hiển thị UID/GID dạng số |
| `-q` | Thay ký tự không in được bằng `?` |
| `-R` | Duyệt thư mục đệ quy |
| `-r` | Đảo ngược thứ tự sắp xếp |
| `-S` | Sắp xếp theo kích thước |
| `-s` | Hiển thị số block đã sử dụng |
| `-t` | Sắp xếp theo thời gian |
| `-u` | Sử dụng thời gian truy cập |
| `-w` | Giữ nguyên ký tự trong tên file |

## 3. Cấu trúc dự án

| File | Chức năng |
|---|---|
| `main.c` | Tiếp nhận tham số và điều phối chương trình |
| `ls.c`, `ls.h` | Đọc thư mục, sắp xếp và duyệt đệ quy |
| `options.c`, `options.h` | Phân tích các tùy chọn dòng lệnh |
| `display.c`, `display.h` | Định dạng và hiển thị metadata |
| `Makefile` | Biên dịch và dọn sản phẩm biên dịch |
| `README.md` | Tài liệu hướng dẫn |
| `.gitignore` | Loại trừ file không cần thiết |

## 4. Biên dịch chương trình

Chương trình được biên dịch trên NetBSD 10.1 bằng trình biên dịch `cc` và BSD Make.

Di chuyển vào thư mục dự án:

`cd /root/midterm_ls`

Dọn sản phẩm biên dịch cũ:

`make clean`

Biên dịch chương trình:

`make`

Sau khi biên dịch thành công, tệp thực thi `myls` sẽ được tạo.

## 5. Hướng dẫn sử dụng

Một số ví dụ:

| Lệnh | Mô tả |
|---|---|
| `./myls` | Liệt kê thư mục hiện tại |
| `./myls -la` | Hiển thị thông tin chi tiết |
| `./myls -lh /etc` | Hiển thị kích thước dễ đọc |
| `./myls -R test_midterm` | Duyệt thư mục con |
| `./myls -S /etc` | Sắp xếp theo kích thước |
| `./myls -F test_midterm` | Phân loại các loại tệp |
| `./myls -sk test_midterm` | Hiển thị số block theo KB |

## 6. Kiểm thử

Chương trình được kiểm thử thủ công trên NetBSD 10.1, bằng cách so sánh với lệnh `ls` gốc.

Các bài kiểm tra đã thực hiện bao gồm:

- Liệt kê file ẩn và hiển thị thông tin chi tiết bằng `-la`.
- Phân loại file và thư mục bằng `-F`.
- Duyệt các thư mục con bằng `-R`.
- Sắp xếp theo kích thước và đảo thứ tự bằng `-Sr`.
- Hiển thị chính thư mục bằng `-ld`.
- Xử lý đơn vị block bằng `-sk` và biến môi trường `BLOCKSIZE`.
- Hiển thị nhiều đường dẫn trong cùng một lệnh.

Các kết quả thử nghiệm cơ bản đã cho thấy chương trình hoạt động phù hợp với lệnh `ls` gốc trong các trường hợp được kiểm tra. Một số trường hợp đặc biệt vẫn cần kiểm thử thêm.

## 7. Kết quả đạt được

- Xây dựng chương trình MYLS bằng ngôn ngữ C.
- Tổ chức chương trình thành nhiều mô đun `.c` và `.h`.
- Xây dựng mã xử lý cho 19 tùy chọn.
- Biên dịch chương trình thành công trên NetBSD 10.1.
- Thực hiện kiểm thử các chức năng liệt kê, sắp xếp và duyệt đệ quy.
- Sử dụng Makefile để tự động biên dịch chương trình.

## 8. Kết luận

Thông qua đề tài, em đã tìm hiểu và vận dụng các kiến thức lập trình hệ thống trên UNIX, đặc biệt là các thao tác truy cập thư mục, quản lý metadata, xử lý đối số dòng lệnh và tổ chức chương trình theo mô đun.

Đề tài giúp em nâng cao kỹ năng lập trình C, sử dụng NetBSD, biên dịch bằng Makefile và quản lý mã nguồn với GitHub.

## 9. Repository

https://github.com/Mnhat26/HoangMinhNhat_24IT189
