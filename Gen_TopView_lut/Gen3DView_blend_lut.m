%功能1: 使用了最大成像面曲面划分方法，并生成纹理坐标与顶点坐标
%改动1: 修改了旋转角->1.棋盘到世界坐标系， 2.车身到世界坐标系,保持运动方向与全景系统一致
%改动2: 将车身坐标系统一改为与右手坐标系系统一致，行车方向向前为X方向，向右为X轴方向，向上为Z轴方向
% 改动3: 曲面划分时，按照从俯视视图将曲面划分为四个部分
% 0724: 修正了棋盘坐标系到车身坐标系旋转平移矩阵计算过程，修正了gen_panel2world函数
% 0724: 相机遍历顺序变为 前->右->后->左
% 0821: 新增从3D文件导入顶点及曲面信息
%                                front_cam
%                        |-----------------------mesh gride end
%                        | Quadrant1 | Quadrant2 |
%             left_cam   |-----------------------| right_cam
%                        | Quadrant4 | Quadrant3 |
%          mesh gride start----------------------|
%                                rear_cam

close all;clc;clear;%清除数据

%*****************************输入参数*************************************
render_mode  = 1;%0 单三角形绘制, 1 三角形连续绘制
pc_sim =0;       %1 用于PC仿真,0 用于TDA2X移植
view_style   = 0;%0 top view 1 3D-view
draw_enable  = 0;

%棋盘尺寸,标定场地尺寸
squar_size     = 600;%mm
num_h          = 13; %标定场地中车身侧边从前到后所用的棋盘格子数
num_w          = 8;  %标定场地中车身前/后从左到右所用的棋盘格子数

% calibrate_path = '.\0308_calibrate\'; %标定数据路径
% calibrate_path = './0414_calibrate_HY/';
calibrate_path   = './aions0818/';
dist = 3000;% unit mm

quad_num = 4;%将平面分成4个区域
sub_x =4;    %水平方向间隔像素
sub_y =4;    %垂直方向间隔像素

resolution_H =1080;
resolution_W =1080;

gride_H  = resolution_H/sub_y/2;%必须是2的整数倍,表示横向被分为N个格子   1080 -> 270
gride_W  = resolution_W/sub_x/2;%必须是2的整数倍,表示纵向被分为N个格子   1080 -> 270

sight_H    = 15000;             %unint mm 15米可视区域
sight_W    = 15000;             %unint mm 15米可视区域含车辆

quad_sight_h = sight_H/2;%quad区域可视范围
quad_sight_w = sight_W/2;%quad区域可视范围

gride_gap_h = quad_sight_h/gride_H; % unit: mm，使用犀牛软件进行绘制时，选择厘米为单位
gride_gap_w = quad_sight_w/gride_W;


% corner position 
% o1 ________ o2
% |\         /|
% |  i1___i2  |
% |  |     |  |
% |  i4___i3  |
% | /       \ |
% o4_________ o3

outer_corner = [-sight_W/2, sight_H/2;  %corner o1
                 sight_W/2, sight_H/2;  %corner o2
                 sight_W/2,-sight_H/2;  %corner o3
                -sight_W/2,-sight_H/2;];%corner o4
            
car_size = [4700,2100]; %大通车身尺寸

inter_corner = [-car_size(2)/2, car_size(1)/2;
                 car_size(2)/2, car_size(1)/2;
                 car_size(2)/2,-car_size(1)/2;
                -car_size(2)/2,-car_size(1)/2;];

angle_offset =15;

% car_size = [4720,2100];%汉兰达车身尺寸
car_size = [4500,2000];  %Aions

%************************从mat文件中获取,初始化外参************************
datapath   = dir(strcat(calibrate_path,'*.mat'));
para_front = load([calibrate_path,datapath(1).name]);
para_left  = load([calibrate_path,datapath(2).name]);
para_rear  = load([calibrate_path,datapath(3).name]);
para_right = load([calibrate_path,datapath(4).name]);

para = {para_front,para_right,para_rear,para_left}; %顺时针

%*********************从TXT中获取,初始化相机内参***************************
instrinc_datapath = dir(strcat(calibrate_path,'*.txt'));
front_model = get_Calibrate([calibrate_path,instrinc_datapath(1).name]);
left_model  = get_Calibrate([calibrate_path,instrinc_datapath(2).name]);
rear_model  = get_Calibrate([calibrate_path,instrinc_datapath(3).name]);
right_model = get_Calibrate([calibrate_path,instrinc_datapath(4).name]);

model = {front_model,right_model,rear_model,left_model};%顺时针

%**************************读入图像****************************************
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

[R_p2cam(:,:,1),R_cam2p(:,:,1)] = gen_Rotation(para_front.calib_data.RRfin);    %前
[R_p2cam(:,:,2),R_cam2p(:,:,2)] = gen_Rotation(  para_right.calib_data.RRfin);  %右
[R_p2cam(:,:,3),R_cam2p(:,:,3)] = gen_Rotation(  para_rear.calib_data.RRfin);   %后
[R_p2cam(:,:,4),R_cam2p(:,:,4)] = gen_Rotation(para_left.calib_data.RRfin);     %左


Pos_4cam_onP(:,1)  = R_cam2p(:,:,1)*[0;0;0;1];% 前相机在前棋盘坐标系中的位置
Pos_4cam_onP(:,2)  = R_cam2p(:,:,2)*[0;0;0;1];% 右相机在前棋盘坐标系中的位置
Pos_4cam_onP(:,3)  = R_cam2p(:,:,3)*[0;0;0;1];% 后相机在前棋盘坐标系中的位置
Pos_4cam_onP(:,4)  = R_cam2p(:,:,4)*[0;0;0;1];% 左相机在前棋盘坐标系中的位置

%******************************生成1/4平面区域*******************************


% X-coord Y-coord ,1-latitude coord
gride_points(1:gride_H+1,1:gride_W+1,1:3) = 0;
%先行后列
for i = 1:gride_H+1
    for j = 1:gride_W+1
        gride_points(i,j,1)   = (i-1)*(gride_W+1) +j;%point_num
        gride_points(i,j,2:3) = [gride_gap_w*(j-1),gride_gap_h*(i-1)];%[水平为x，垂直为y]
    end
end

size_array = size(gride_points);
points_reshape = reshape(gride_points,size_array(1)*size_array(2),3);
points_reshape = sortrows(points_reshape,1);%行向量按矩阵第一列进行递增排列

%*****************************创建网格坐标***********************************
%                                front_cam0
%                        |-----------------------mesh gride end
%                        | Quadrant1 | Quadrant2 |
%            left_cam3   |-----------------------| right_cam1
%                        | Quadrant4 | Quadrant3 |
%          mesh gride start----------------------|
%                                rear_cam2
if pc_sim ==1
for i = 1:quad_num
    if i==1
    quad1 = [points_reshape(:,1),points_reshape(:,2)-quad_sight_h,points_reshape(:,3)];
    end
    if i==2
    quad2 = [points_reshape(:,1),points_reshape(:,2),points_reshape(:,3)];
    end
    if i==3
    quad3 = [points_reshape(:,1),points_reshape(:,2),points_reshape(:,3)-quad_sight_w];
    end
    if i==4
    quad4 = [points_reshape(:,1),points_reshape(:,2)-quad_sight_h,points_reshape(:,3)-quad_sight_w];
    end
end
end

if pc_sim ==0
for i = 1:quad_num
    if i==1
    quad1 = [points_reshape(:,1),points_reshape(:,2)-quad_sight_w,quad_sight_h-points_reshape(:,3)];
    end
    if i==2
    quad2 = [points_reshape(:,1),points_reshape(:,2),             quad_sight_h-points_reshape(:,3)];
    end
    if i==3
    quad3 = [points_reshape(:,1),points_reshape(:,2),                         -points_reshape(:,3)];
    end
    if i==4
    quad4 = [points_reshape(:,1),points_reshape(:,2)-quad_sight_w,            -points_reshape(:,3)];
    end
end
end

%**************************显示网点区域分割********************************
%实际各区域点数量相等，后绘制的点颜色会将先绘制的点覆盖
%红色 -quadrant1 ;绿色 -quadrant2 ;蓝色 -quadrant3 ;粉色 -quadrant4
color_def ={[1 0 0],[0 1 0],[0 0 1],[1 0 1]};
if draw_enable ==1
figure
plot(quad1(:,2),quad1(:,3) ,'.','Color',color_def{1});
hold on
plot(quad2(:,2),quad2(:,3) ,'o','Color',color_def{2});
hold on
plot(quad3(:,2),quad3(:,3) ,'*','Color',color_def{3});
hold on
plot(quad4(:,2),quad4(:,3) ,'o','Color',color_def{4});

end

% lut_union  = [ quad1; quad2; quad3; quad4 ];
% size_quad  = [size(quad1);size(quad2);size(quad3);size(quad4)];
% 
% if view_style == 1 %3D view下计算Z轴数值
%    deep_result = deep_cal(lut_union(:,2:3),inter_corner,car_size,dist);
% end
% 
% 
% quad1(:,2:4) = deep_result(1:size_quad(1,1),1:3);
% quad2(:,2:4) = deep_result(1+size_quad(1,1):size_quad(1,1)+size_quad(2,1),1:3);
% quad3(:,2:4) = deep_result(1+size_quad(1,1)+size_quad(2,1):size_quad(1,1)+size_quad(2,1)+size_quad(3,1),1:3);
% quad4(:,2:4) = deep_result(1+size_quad(1,1)+size_quad(2,1)+size_quad(3,1):size_quad(1,1)+size_quad(2,1)+size_quad(3,1)+size_quad(4,1),1:3);

quad       = { quad1, quad2, quad3, quad4 }; %顺时针排列

srv_lut =[];

%**********************生成纹理坐标****************************************
figure
for i=0:3
    % col 1:quad num; col 2: point_index ;col 3~5: vertex x,y,z;
    quad_temp = quad{i+1};
    LUT_quad(:,2:4) = quad_temp(:,1:3);
    if view_style == 0
        LUT_quad(:,5) = 0;
    else
        LUT_quad(:,5) = quad_temp(:,4);
    end
    LUT_quad(:,1)=i+1;%标记quad值
    quad_temp =[];%释放中的数据，避免由于各quad中元素数量不同而导致冲突
    % col 5~6: u1v1 , col 7~8: u2v2 ,col 9~10: blend 1,blend 2
    %构造网格点世界坐标
    world_pos =LUT_quad(:,3:5);world_pos(:,4) =1;%齐次化
    %计算当前quad中网格点在左侧相机中得纹理点坐标
    cam_temp2 = R_p2cam(:,:,rem(i+3,4)+1)*R_w2p(:,:,rem(i+3,4)+1)*world_pos';   %转换为棋盘坐标
    uv2   = world2cam(cam_temp2(1:3,:),model{rem(i+3,4)+1});  %world2cam 从相机坐标系到成像面坐标系
    
    %----------越界处理-----------
    ind_temp = uv2(1,:)>img_height;uv2(1,ind_temp) =img_height;ind_temp =[];
    ind_temp = uv2(1,:)< 0;        uv2(1,ind_temp) =0;         ind_temp =[];
    ind_temp = uv2(2,:)>img_width; uv2(2,ind_temp) =img_width; ind_temp =[];
    ind_temp = uv2(2,:)< 0;        uv2(2,ind_temp) =0;         ind_temp =[];
    
    %计算当前quad中网格点在右侧相机中得纹理点坐标
    cam_temp1 = R_p2cam(:,:,i+1)*R_w2p(:,:,i+1)*world_pos';   %转换为棋盘坐标
    uv1   = world2cam(cam_temp1(1:3,:),model{i+1});  %world2cam 从相机坐标系到成像面坐标系
    %----------越界处理-----------
    ind_temp = uv1(1,:)>img_height;uv1(1,ind_temp) =    img_height;ind_temp =[];
    ind_temp = uv1(1,:)< 0;        uv1(1,ind_temp) =    0;         ind_temp =[];
    ind_temp = uv1(2,:)>img_width; uv1(2,ind_temp) =    img_width; ind_temp =[];
    ind_temp = uv1(2,:)< 0;        uv1(2,ind_temp) =    0;         ind_temp =[];
    
    %显示
    subplot(4,2,2*i+1);
    im = img_src{rem(i+3,4)+1};imshow(im);text(0,40,img_tag{rem(i+3,4)+1},'Color',[0 1 0]);hold on;axis on; 
    plot(uv2(2,:),uv2(1,:) ,'.','Color',color_def{i+1});
    
    subplot(4,2,2*i+2);  
    im = img_src{i+1};imshow(im);text(0,40,img_tag{i+1},'Color',[0 1 0]);hold on;axis on; 
    plot(uv1(2,:),uv1(1,:) ,'.','Color',color_def{i+1});  
    
    LUT_quad(:,6:9) = [uv1',uv2'];
    LUT_quad(:,10:11) = 0.5;%初始纹理权重为0.5
    srv_lut = [srv_lut;LUT_quad]; 
    LUT_quad =[];%释放缓存数据
end

%srv_lut = sortrows(srv_lut,2); %不再重排序列

%*****************************计算混合权重***********************************

srv_lut = blend_cal(srv_lut,outer_corner,inter_corner,angle_offset);

%*****************************生三角形索引***********************************

if pc_sim == 1
% Trg_indices = Gen_indices(gride_H,gride_W,'GL_TRIANGLE');
    Trg_indices = Gen_indices_fast(points_reshape(:,1),gride_H,gride_W);
%按点重排坐标，总点数量为(gride_H+1)*(gride_W+1)
%Trg_indices = sortrows(Trg_indices,1);
else
    Trg_indices = Gen_indices_VisionSDK(136,136,1);
    Trg_indices = Trg_indices';
end
disp('finished Generate Top_View_blend_lut');

%************************绘制三维mesh***************************************
% %运行速度慢，在使用前，先降低gride_W和gride_H的值,遍面程序运行过慢
% if draw_enable == 1color_def
% figure
% X =[(srv_lut(Trg_indices(:,1),3))';(srv_lut(Trg_indices(:,2),3))';(srv_lut(Trg_indices(:,3),3))'];
% Y =[(srv_lut(Trg_indices(:,1),4))';(srv_lut(Trg_indices(:,2),4))';(srv_lut(Trg_indices(:,3),4))'];
% Z =[(srv_lut(Trg_indices(:,1),5))';(srv_lut(Trg_indices(:,2),5))';(srv_lut(Trg_indices(:,3),5))'];
% fill3(X,Y,Z, 'g')
% end

%*************************数值处理******************************************
if pc_sim ==1
    scaler_u = 1/1280;
    scaler_v = 1/720;
    scaler_x = 0.0714/540;
    scaler_y = 0.0714/540;
    scaler_z = 1;
    scaler_blend = 1;
else
    scaler_u = 16;% TDA2X visionSDK uv divide rang :u 1280*16 =20480 
    scaler_v = 16;%                                 v 720*16  =11520 
    scaler_x = 0.0714;
    scaler_y = 0.0714;
    scaler_z = 0.0714;
    scaler_blend = 255;
end

srv_lut(:,3) = srv_lut(:,3)*scaler_x;
srv_lut(:,4) = srv_lut(:,4)*scaler_y;
srv_lut(:,5) = srv_lut(:,5)*scaler_z;

srv_lut(:,6) = srv_lut(:,6)*scaler_v;
srv_lut(:,7) = srv_lut(:,7)*scaler_u;

srv_lut(:,8) = srv_lut(:,8)*scaler_v;
srv_lut(:,9) = srv_lut(:,9)*scaler_u;

srv_lut(:,10) = srv_lut(:,10)*scaler_blend;
srv_lut(:,11) = srv_lut(:,11)*scaler_blend;

%**************************保存为二级制文件***********************************
%写入 lut
%fwrite 默认将矩阵按先列后行写入
fid = fopen('srv_lut.bin','wb');
if pc_sim ==1
    srv_lut_wrdata = srv_lut(:,3:11)';
    fwrite(fid,srv_lut_wrdata,'float');
end
if pc_sim ==0
    srv_lut_wrdata = round(srv_lut(:,3:9)');%不反置,在GLSL中进行反置
%   srv_lut_wrdata = round([srv_lut(:,3:5),srv_lut(:,7),srv_lut(:,6),srv_lut(:,9),srv_lut(:,8)]');%uv位置反置
    fwrite(fid,srv_lut_wrdata,'short');
end
fclose(fid);

%read back for check
fid = fopen('srv_lut.bin','rb');

if pc_sim ==1
    A = fread(fid,9*100,'float');
    A = reshape(A,9,100);
end
if pc_sim ==0
    A = fread(fid,7*100,'short');
    A = reshape(A,7,100);
end

%写入blend
fid = fopen('srv_blend_lut.bin','wb');
if pc_sim ==1
    srv_blend_lut_wrdata = srv_lut(:,10:11)';
    fwrite(fid,srv_blend_lut_wrdata,'float');
end
if pc_sim ==0
    srv_blend_lut_wrdata = round(srv_lut(:,10:11)');
    fwrite(fid,srv_blend_lut_wrdata,'unsigned char');
end
fclose(fid);

%read back for check
fid = fopen('srv_blend_lut.bin','rb');
if pc_sim ==1
    B = fread(fid,2*100,'float');
    B = reshape(B,2,100);
end
if pc_sim ==0
    B = fread(fid,2*100,'unsigned char');
    B = reshape(B,2,100);
end

%写入indices
fid = fopen('srv_indices.bin','wb');
if pc_sim == 1
    srv_indices_wrdata = Trg_indices(:,1:3)'-1;%opengGL 索引从0开始，易导致三角形错位
else
    srv_indices_wrdata = Trg_indices();        %第一个数据为index总长度
end
fwrite(fid,srv_indices_wrdata,'unsigned int');
fclose(fid);

%read back for check
fid = fopen('srv_indices.bin','rb');
if pc_sim == 1
    C = fread(fid,3*100,'unsigned int');
    C = reshape(C,3,100);
else
    C = fread(fid,3*100,'unsigned int');
end

