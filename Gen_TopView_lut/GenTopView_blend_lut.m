%功能1: 使用了最大成像面曲面划分方法，并生成纹理坐标与顶点坐标
%改动1: 修改了旋转角->1.棋盘到世界坐标系， 2.车身到世界坐标系,保持运动方向与全景系统一致
%改动2: 将车身坐标系统一改为与右手坐标系系统一致，行车方向向前为X方向，向右为X轴方向，向上为Z轴方向
%改动3: 曲面划分时，按照从俯视视图将曲面划分为四个部分
% 0724: 修正了棋盘坐标系到车身坐标系旋转平移矩阵计算过程，修正了gen_panel2world函数
% 0724: 相机遍历顺序变为 前->右->后->左
%                                front_cam
%                        |-----------------------mesh gride end
%                        | Quadrant1 | Quadrant2 |
%             left_cam   |-----------------------| right_cam
%                        | Quadrant4 | Quadrant3 |
%          mesh gride start----------------------|
%                                rear_cam

close all;clc;clear;%清除数据

% yuvRead('rear',...
%         './byd_demo0909/',...
%         'NV12',720,1280,...
%         60,...
%         './byd_demo0909/');

draw_mesh =0;
%*****************************输入参数*************************************

%棋盘尺寸,标定场地尺寸
% squar_size     = 600;%mm
% num_h          = 13; %标定场地中车身侧边从前到后所用的棋盘格子数
% num_w          = 8;  %标定场地中车身前/后从左到右所用的棋盘格子数

squar_size = 24;
num_h =23;
num_w =19;

%交叠区域尺寸
% combine_dist   = 500;
combine_dist   = 0;

%红色 -quadrant1 ;绿色 -quadrant2 ;蓝色 -quadrant3 ;粉色 -quadrant4
color_def ={[1 0 0],[0 1 0],[0 0 1],[1 0 1]};


% calibrate_path = './aions0818/';
calibrate_path = './byd_demo0909/';

%3d view
% src_path_q1 = './3d_surface/td_q1.obj';
% src_path_q2 = './3d_surface/td_q2.obj';
% src_path_q3 = './3d_surface/td_q3.obj';
% src_path_q4 = './3d_surface/td_q4.obj';
src_path_q1 = './byd_demo0909/td_q1.obj';
src_path_q2 = './byd_demo0909/td_q2.obj';
src_path_q3 = './byd_demo0909/td_q3.obj';
src_path_q4 = './byd_demo0909/td_q4.obj';

% topview
% src_path_q1 = './2d_surface/td_q1.obj';
% src_path_q2 = './2d_surface/td_q2.obj';
% src_path_q3 = './2d_surface/td_q3.obj';
% src_path_q4 = './2d_surface/td_q4.obj';


angle_offset =15;

% car_size = [4720,2100];%汉兰达车身尺寸
 car_size = [360,312];%演示盒子尺寸

%************************从mat文件中获取,初始化外参****************************
datapath   = dir(strcat(calibrate_path,'*.mat'));
para_front = load([calibrate_path,datapath(1).name]);
para_left  = load([calibrate_path,datapath(2).name]);
para_rear  = load([calibrate_path,datapath(3).name]);
para_right = load([calibrate_path,datapath(4).name]);

para = {para_front,para_right,para_rear,para_left}; %顺时针

%*********************从TXT中获取,初始化相机内参*******************************
instrinc_datapath = dir(strcat(calibrate_path,'*.txt'));
front_model = get_Calibrate([calibrate_path,instrinc_datapath(1).name]);
left_model  = get_Calibrate([calibrate_path,instrinc_datapath(2).name]);
rear_model  = get_Calibrate([calibrate_path,instrinc_datapath(3).name]);
right_model = get_Calibrate([calibrate_path,instrinc_datapath(4).name]);

model = {front_model,right_model,rear_model,left_model};%顺时针

%**************************读入图像*****************************************
%顺时针排列
img_src{1} = imread([calibrate_path,'/front0.bmp']);%前
img_src{2} = imread([calibrate_path,'/right0.bmp']);%右
img_src{3} = imread([calibrate_path,'/rear0.bmp']); %后
img_src{4} = imread([calibrate_path,'/left0.bmp']); %左
%添加标签项
img_tag ={'前相机图像','右相机图像','后相机图像','左相机图像'};
%获取图像尺寸
size_temp  = size(img_src{1});
img_height = size_temp(1,1);%图像高度
img_width  = size_temp(1,2);%图像宽度

%***************************标定场地参数初始化*****************************
pattern_H  = num_h *squar_size;
pattern_W  = num_w *squar_size;

%计算棋盘系统四个起点在车身系统中的坐标 定义车中心为车身坐标系中心
%向前为Y轴 向右为X轴 向上为Z轴
pos_front = [-pattern_W/2, pattern_H/2];
pos_left  = [-pattern_W/2,-pattern_H/2];
pos_rear  = [ pattern_W/2,-pattern_H/2];
pos_right = [ pattern_W/2, pattern_H/2];

%***************************计算坐标转换矩阵*******************************
%平移矢量:①方向从平移终点指向起点,②长度为其在目标坐标系中的矢量值-0724 
%旋转角度:棋盘坐标系变换到车身坐标系的转换角度
%前
degree =-270;tranform = [pos_front(1,1),pos_front(1,2)];%旋转角度，平移矢量
[R_p2w(:,:,1) ,R_w2p(:,:,1)] = gen_panel2world(degree, tranform(1,1),tranform(1,2)); %前

degree =-180;tranform  = [pos_right(1,1),pos_right(1,2)];%旋转角度，平移矢量
[R_p2w(:,:,2) ,R_w2p(:,:,2)] = gen_panel2world(degree, tranform(1,1),tranform(1,2)); %右

degree =-90;tranform  = [pos_rear(1,1),pos_rear(1,2)];%旋转角度，平移矢量
[R_p2w(:,:,3) ,R_w2p(:,:,3)] = gen_panel2world(degree, tranform(1,1),tranform(1,2)); %后

degree =0;tranform  = [pos_left(1,1),pos_left(1,2)];%旋转角度，平移矢量
[R_p2w(:,:,4) ,R_w2p(:,:,4)] = gen_panel2world(degree, tranform(1,1),tranform(1,2)); %左

[R_p2cam(:,:,1),R_cam2p(:,:,1)] = gen_Rotation(para_front.calib_data.RRfin);   %前
[R_p2cam(:,:,2),R_cam2p(:,:,2)] = gen_Rotation(para_right.calib_data.RRfin); %右
[R_p2cam(:,:,3),R_cam2p(:,:,3)] = gen_Rotation(para_rear.calib_data.RRfin);   %后
[R_p2cam(:,:,4),R_cam2p(:,:,4)] = gen_Rotation(para_left.calib_data.RRfin);     %左

Pos_4cam_onP(:,1)  = R_cam2p(:,:,1)*[0;0;0;1];% 前相机在前棋盘坐标系中的位置
Pos_4cam_onP(:,2)  = R_cam2p(:,:,2)*[0;0;0;1];% 右相机在前棋盘坐标系中的位置
Pos_4cam_onP(:,3)  = R_cam2p(:,:,3)*[0;0;0;1];% 后相机在前棋盘坐标系中的位置
Pos_4cam_onP(:,4)  = R_cam2p(:,:,4)*[0;0;0;1];% 左相机在前棋盘坐标系中的位置


src_path ={src_path_q1,src_path_q2,src_path_q3,src_path_q4};%quad1,quad2,quad3,quad4
for i = 1:4
    [mask_vertex{i},num_v(i),index{i},num_f(i)] = gen_srv_lut_from3Dfile(src_path{i},[10,10,10]);
end


if draw_mesh ==1
    for i =1:4
    figure
    X=          [mask_vertex{i}(index{i}(:,1),2),mask_vertex{i}(index{i}(:,2),2),mask_vertex{i}(index{i}(:,3),2)];
                      
    Y=          [mask_vertex{i}(index{i}(:,1),3),mask_vertex{i}(index{i}(:,2),3),mask_vertex{i}(index{i}(:,3),3)];
         
    Z=          [mask_vertex{i}(index{i}(:,1),4),mask_vertex{i}(index{i}(:,2),4),mask_vertex{i}(index{i}(:,3),4)];

    temp_size = size(X);

    C = ones(temp_size);

    fill3(X',Y',Z',C');
    X=[];Y=[];Z=[];C=[];
    end
end



figure
for i = 0:3
% col 1:quad num; col 2: point_index ;col 3~5: vertex x,y,z;
%构造网格点世界坐标
srv_lut{i+1}(:,1:3) = mask_vertex{i+1}(:,2:4);
world_pos =mask_vertex{i+1}(:,2:4);world_pos(:,4) =1;%齐次化
%计算当前quad中网格点在左侧相机中得纹理点坐标
cam_temp2 = R_p2cam(:,:,rem(i+3,4)+1)*R_w2p(:,:,rem(i+3,4)+1)*world_pos'; %转换为棋盘坐标
uv2   = world2cam(cam_temp2(1:3,:),model{rem(i+3,4)+1});  %world2cam 从相机坐标系到成像面坐标系
    
%----------越界处理-----------
ind_temp = uv2(1,:)>img_height;uv2(1,ind_temp) =0; ind_temp =[];
ind_temp = uv2(1,:)< 0;        uv2(1,ind_temp) =0; ind_temp =[];
ind_temp = uv2(2,:)>img_width; uv2(2,ind_temp) =0; ind_temp =[];
ind_temp = uv2(2,:)< 0;        uv2(2,ind_temp) =0; ind_temp =[];
    
%计算当前quad中网格点在右侧相机中得纹理点坐标
cam_temp1 = R_p2cam(:,:,i+1)*R_w2p(:,:,i+1)*world_pos';   %转换为棋盘坐标
uv1   = world2cam(cam_temp1(1:3,:),model{i+1});  %world2cam 从相机坐标系到成像面坐标系
%----------越界处理-----------
ind_temp = uv1(1,:)>img_height;uv1(1,ind_temp) =0;ind_temp =[];
ind_temp = uv1(1,:)< 0;        uv1(1,ind_temp) =0;ind_temp =[];
ind_temp = uv1(2,:)>img_width; uv1(2,ind_temp) =0;ind_temp =[];
ind_temp = uv1(2,:)< 0;        uv1(2,ind_temp) =0;ind_temp =[];
    
%显示
subplot(4,2,2*i+1);
im = img_src{rem(i+3,4)+1};imshow(im);text(0,40,img_tag{rem(i+3,4)+1},'Color',[0 1 0]);hold on;axis on; 
plot(uv2(2,:),uv2(1,:) ,'.','Color',color_def{i+1});
 
subplot(4,2,2*i+2);  
im = img_src{i+1};imshow(im);text(0,40,img_tag{i+1},'Color',[0 1 0]);hold on;axis on; 
plot(uv1(2,:),uv1(1,:) ,'.','Color',color_def{i+1});  
    
srv_lut{i+1}(:,4:7) = [uv1',uv2'];
srv_lut{i+1}(:,8:9) = 0.5;%初始纹理权重为0.5

end

%srv_lut = sortrows(srv_lut,2); %不需重排序列

%*****************************计算混合权重***********************************

for i=1:4
     srv_lut{i} = blend_cal(i,srv_lut{i},car_size,angle_offset);
end

figure
for i=1:4
    scatter3(srv_lut{i}(:,1),srv_lut{i}(:,2),srv_lut{i}(:,3),50,color_def{i},'filled')
    hold on
end

%********************************顶点列表***********************************
vertex_lut_length1 = size(srv_lut{1});
vertex_lut_length2 = size(srv_lut{2});
vertex_lut_length3 = size(srv_lut{3});
vertex_lut_length4 = size(srv_lut{4});

srv_lut_wr_length = [vertex_lut_length1(1);
                     vertex_lut_length2(1);
                     vertex_lut_length3(1);
                     vertex_lut_length4(1)];
                 
srv_lut_data   = [srv_lut{1};srv_lut{2};srv_lut{3};srv_lut{4}];

%*****************************三角形索引列表**********************************
index_length1 = size(index{1});
index_length2 = size(index{2});
index_length3 = size(index{3});
index_length4 = size(index{4});

indices_wr_length = [index_length1(1);index_length2(1);index_length3(1);index_length4(1)];
indices_wr_data   = [index{1};index{2};index{3};index{4}];

%*******************************数值处理*************************************

scaler_u = 16;% TDA2X visionSDK uv divide rang :u 1280*16 =20480 
scaler_v = 16;%                                 v 720*16  =11520 

% scaler_x = 0.0714;
% scaler_y = 0.0714;
% scaler_z = 0.0714;

scaler_x = 0.714;
scaler_y = 0.714;
scaler_z = 0.714;

scaler_blend = 255;

srv_lut_data(:,1) = srv_lut_data(:,1)*scaler_x;
srv_lut_data(:,2) = srv_lut_data(:,2)*scaler_y;
srv_lut_data(:,3) = srv_lut_data(:,3)*scaler_z;

srv_lut_data(:,4) = srv_lut_data(:,4)*scaler_v;
srv_lut_data(:,5) = srv_lut_data(:,5)*scaler_u;

srv_lut_data(:,6) = srv_lut_data(:,6)*scaler_v;
srv_lut_data(:,7) = srv_lut_data(:,7)*scaler_u;

srv_lut_data(:,8) = srv_lut_data(:,8)*scaler_blend;
srv_lut_data(:,9) = srv_lut_data(:,9)*scaler_blend;

%**************************保存为二级制文件***********************************
%----------------写入 lut----------------------
%fwrite 默认将矩阵按先列后行写入
fid = fopen('srv_lut.bin','wb');

srv_lut_wr_data = srv_lut_data(:,1:7)';%位置坐标精度损失
fwrite(fid,srv_lut_wr_length,'unsigned int');
fwrite(fid,srv_lut_wr_data,'short');
fclose(fid);

temp_lut_size =size(srv_lut_wr_data);

%read back for check
fid = fopen('srv_lut.bin','rb');
ansvertex_numget = fread(fid,4,'unsigned int');
ans_srv_lut_read = fread(fid,temp_lut_size(1)*temp_lut_size(2),'short');
ans_srv_lut_read = reshape(ans_srv_lut_read,temp_lut_size(1),temp_lut_size(2));
fclose(fid);

%comfirm how the bow works like
figure
scatter3(ans_srv_lut_read(1,:),ans_srv_lut_read(2,:),ans_srv_lut_read(3,:))

%----------------写入blend--------------------
fid = fopen('srv_blend_lut.bin','wb');  
fwrite(fid,srv_lut_wr_length,'unsigned int');
srv_blend_lut_wr_data = round(srv_lut_data(:,8:9)');
fwrite(fid,srv_blend_lut_wr_data,'unsigned char');
fclose(fid);

%read back for check
fid = fopen('srv_blend_lut.bin','rb');
ans_blend_numget = fread(fid,4,'unsigned int');
ans_blend_read= fread(fid,2*300,'unsigned char');
ans_blend_read = reshape(ans_blend_read,2,300);
fclose(fid);

%---------------写入indices-------------------
fid = fopen('srv_indices.bin','wb');
fwrite(fid,indices_wr_length,'unsigned long');
fwrite(fid,indices_wr_data'-1,'unsigned int');%转换到C代码从0开始，matlab索引从1开始
fclose(fid);

%read back for check
fid = fopen('srv_indices.bin','rb');
ans_index_numget = fread(fid,4,'unsigned long');
ans_index_read = fread(fid,3*300,'unsigned int');
ans_index_read = reshape(ans_index_read,3,300)';
fclose(fid);


disp("finished gen srv_lut,pls copy srv,blend,index bin files to TDA2X")
