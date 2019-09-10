%棋盘坐标系到车身中心所建立的世界坐标系的坐标变换
%输出参数，angle 绕Z轴旋转角度 Xt，Yt，Zt 坐标系原点平移向量
%Xp Yp Zp 棋盘坐标系下的坐标
%Xw Yw Zw车身世界坐标系下的坐标
%RRpw 齐次化旋转平移矩阵

function [R,inv_R] = gen_panel2world(angle,tx,ty)

R    =[  cosd(angle), sind(angle),      0,          tx;
        -sind(angle), cosd(angle),      0,          ty;
         0          ,           0,      1,           0;
         0          ,           0,      0,           1 ];
inv_R =inv(R);