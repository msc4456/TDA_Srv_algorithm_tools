

%WORLD2CAM projects a 3D point on to the image
%   m=WORLD2CAM_FAST(M, ocam_model) projects a 3D point on to the
%   image and returns the pixel coordinates. This function uses an approximation of the inverse
%   polynomial to compute the reprojected point. Therefore it is very fast.
%   
%   M is a 3xN matrix containing the coordinates of the 3D points: M=[X;Y;Z]
%   "ocam_model" contains the model of the calibrated camera.
%   m=[rows;cols] is a 2xN matrix containing the returned rows and columns of the points after being
%   reproject onto the image.
%   
%   Copyright (C) 2008 DAVIDE SCARAMUZZA, ETH Zurich  
%   Author: Davide Scaramuzza - email: davide.scaramuzza@ieee.org

function m = world2cam(M, ocam_model)

xc = ocam_model.xc;
yc = ocam_model.yc;
c = ocam_model.c;
d = ocam_model.d;
e = ocam_model.e;
pol = ocam_model.pol;

npoints = size(M, 2);
theta = zeros(1,npoints);

%求三维坐标点的矢量长度
NORM = sqrt(M(1,:).^2 + M(2,:).^2);

%找出长度为0的矢量
ind0 =  NORM == 0; %these are the scene points which are along the z-axis

%矢量长度为零的点赋一个极小值方便进行计算
NORM(ind0) = eps; %this will avoid division by ZERO later

%求相机坐标系中的沿入射光线与主轴的夹角 atan-反切，代入值为弧度
 theta = atan( M(3,:)./NORM );
%  theta = atan( NORM ./M(3,:) );
%通过夹角与K系数获得投影光线与主轴的夹角
pol = pol(2:end);
pol = pol(end:-1:1);
rho = polyval( pol , theta ); %Distance in pixel of the reprojected points from the image center

%xs/rd  = X/r => xs =X/r *rd 
% r = NORM  rd = rho x=xs
% K系数此处关联的是入射角与投影高度rd
x = M(1,:)./NORM.*rho ;
y = M(2,:)./NORM.*rho ;

%Add center coordinates
m(1,:) = x*c + y*d + xc;
m(2,:) = x*e + y   + yc;

