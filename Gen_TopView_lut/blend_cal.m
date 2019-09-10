%计算纹理混叠权重
%输入参数：srv_lut顶点坐标,inter_corner车身四边顶点，angle_offset混合区域极坐标转角范围
%输出参数：srv_lut_out 包含混合权重的表格

%备注：angle_offset < 45度

% corner position 
% o1 ________ o2
% |\         /|
% |  i1___i2  |
% |  |     |  |
% |  i4___i3  |
% | /       \ |
% o4_________ o3

function srv_lut_out =blend_cal(quad_num,srv_lut,car_size,angle_offset)

ground_H = max(abs(srv_lut(:,2)));
ground_W = max(abs(srv_lut(:,1)));

outer_corner = [-ground_W, ground_H;
                 ground_W, ground_H;
                 ground_W,-ground_H;
                -ground_W,-ground_H ];

inter_corner = [-car_size(2)/2, car_size(1)/2;
                 car_size(2)/2, car_size(1)/2;
                 car_size(2)/2,-car_size(1)/2;
                -car_size(2)/2,-car_size(1)/2;];            

%分割角线矢量表示
corner_vector1 = outer_corner(1,:)-inter_corner(1,:);
corner_vector2 = outer_corner(2,:)-inter_corner(2,:);
corner_vector3 = outer_corner(3,:)-inter_corner(3,:);
corner_vector4 = outer_corner(4,:)-inter_corner(4,:);
corner_vector = [corner_vector1;corner_vector2;corner_vector3;corner_vector4];

inputA = corner_vector(quad_num,:);

%以车四边角点为坐标原点,计算quad中的点坐标矢量表示
inputB = srv_lut(:,1:2)-inter_corner(quad_num,1:2);

size_temp  = size(inputB);

% degree = degree-74;
%//degree = degree^3;
%dis=1/norm(inputA,2)*1000

% v1(x1,y1),v2(x2,y2), x1y2-x2y1 = cross2(v1,v2)
% 值为正，则根据右手法则，从v1转向v2为逆时针，反之为顺时针
% V1xV2 >0 则V2在V1的逆时针方向
polarity = inputA(:,1).*inputB(:,2) -inputB(:,1).*inputA(:,2);

polarity(polarity(:)>0)  =  1;
polarity(polarity(:)<0)  = -1;
polarity(polarity(:)==0) =  0;


degree =[];

for i = 1:size_temp(1,1)
    cos_degree = dot(inputA,inputB(i,:))/norm(inputA,2)/norm(inputB(i,:),2);
    degree =[degree;rad2deg(acos(cos_degree))];
end

degree = degree.*polarity;%获得带方向的角度 逆时针为正

ind_positive = degree(:,1) > angle_offset; 
srv_lut(ind_positive,9) = 1;
srv_lut(ind_positive,8) =0;
ind_positive =[];

ind_positive = degree(:,1)< (0-angle_offset); 
srv_lut(ind_positive,9) = 0;
srv_lut(ind_positive,8) = 1;
ind_positive =[];

ind_positive = degree(:,1)<=angle_offset & degree(:,1)>=(0-angle_offset);
temp_blend  = (degree(ind_positive,1)+angle_offset)/(2*angle_offset);
srv_lut(ind_positive,9) = temp_blend;
srv_lut(ind_positive,8)  = 1-temp_blend;

srv_lut_out = srv_lut;

