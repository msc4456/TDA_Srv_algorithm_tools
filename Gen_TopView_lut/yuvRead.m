%从TDA2x存储的NV12数据中抽取图像
function yuvRead(ch_name,...
                 src_path,...
                 mode,row,col,...
                 frame_length,...
                 dst_path)
% src_path,图像源地址
% mode, 'YV12' or 'NV12' refrence
% row --->height  图像高度
% col --->width   图像宽度
% frame_length    图像帧数  start_frame 截取启始帧,stop_frame截取停止帧

fid = fopen([src_path,ch_name,'.yuv'],'r'); %读入文件
% fod = fopen('YUV_source\edietRight.YUV','w'); %输出文件
% row=720;col=1280; %图像的高、宽
% frame_length=1; % total=97 %序列的帧数

for frame=1:frame_length
  %读入文件 将yuv转换为rgb，并用imshow显示
  %  im_l_y=fread(fid,[row,col],'uchar');  %错误的读入
    im_l_y = zeros(row,col); %Y
    for i1 = 1:row 
       im_l_y(i1,:) = fread(fid,col);  %读取数据到矩阵中 
    end
    
%     YV12
    if strcmp(mode,'YV12')
     im_l_cb = zeros(row/2,col/2); %cb
     for i2 = 1:row/2 
       im_l_cb(i2,:) = fread(fid,col/2);  
     end
     im_l_cr = zeros(row/2,col/2); %cr
     for i3 = 1:row/2
         im_l_cr(i3,:) = fread(fid,col/2);  
     end
    end
    if strcmp(mode,'NV12')
     im_l_cb = zeros(row/2,col/2);
     im_l_cr = zeros(row/2,col/2);
     for i2 = 1:row/2
       im_l_temp(i2,:) = fread(fid,col);
       im_l_cb(i2,:) = im_l_temp(i2,1:2:end); 
       im_l_cr(i2,:) = im_l_temp(i2,2:2:end);  
     end
    end
    %由于输入的yuv文件为4:2:0，所以CbCr要改变大小，
    %否则im_l_ycbcr(:, :, 2) =im_l_cb;会出现错误
    im_l_cb = imresize(im_l_cb, [row, col], 'bicubic');%改变图像的大小
    im_l_cr = imresize(im_l_cr, [row, col], 'bicubic');
    im_l_ycbcr = zeros([row, col, 3]);
    im_l_ycbcr(:, :, 1) = im_l_y;
    im_l_ycbcr(:, :, 2) = im_l_cb;
    im_l_ycbcr(:, :, 3) = im_l_cr;

     dst = ycbcr2rgb(uint8(im_l_ycbcr)); %将YCbCr转换为rgb
    
%     figure, imshow(im_l);title('Down');

%   %输出文件  将rgb转换为yuv，NA
%     im_t_ycbcr = rgb2ycbcr(im_l);  %将rgb转换为YCbCr
%     im_t_y = im_t_ycbcr(:, :, 1);  %Y
%     im_t_cb = im_t_ycbcr(:, :, 2); %Cb
%     im_t_cr = im_t_ycbcr(:, :, 3); %Cr
%     im_l_cb = imresize(im_l_cb, [row/2, col/2], 'bicubic');%改变图像的大小
%     im_l_cr = imresize(im_l_cr, [row/2, col/2], 'bicubic');
%     for i1 = 1:row 
%        fwrite(fod,im_l_y(i1,:));  %输出到文件中
%     end
%     for i1 = 1:row/2
%        fwrite(fod,im_l_cb(i1,:));  
%     end
%     for i1 = 1:row/2
%        fwrite(fod,im_l_cr(i1,:)); 
%     end
      imwrite(dst,[dst_path,ch_name,num2str(frame),'.bmp']);
end
fclose(fid);