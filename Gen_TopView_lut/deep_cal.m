function vertex_pints = deep_cal(lut,inter_corner,car_size,dist)

%    s1   |   s2    |  s3
%---------i1--------i2---------
%    s8   |   s9    |  s4
%---------i4--------i3---------  
%    s7   |   s6    |  s5

%s0 汽车模型/汽车实体位置区域
%i1,i2,i3,i4 车模型/汽车实体四角边界点


% deep_res =0;

vertex_lut = lut;
dist2 = dist^2;
x = car_size(2)/2;
y = car_size(1)/2;

s{9}= find(vertex_lut(:,1)>-x & vertex_lut(:,1)<x &...
     vertex_lut(:,2)>-y & vertex_lut(:,2)<y);%车模型放置区域
 
s{1} = find(vertex_lut(:,1)<=-x & vertex_lut(:,2) >= y);%

s{2} = find(vertex_lut(:,1)>=-x & vertex_lut(:,1) <= x & vertex_lut(:,2)>=y);

s{3} = find(vertex_lut(:,1)>= x & vertex_lut(:,2) >= y);%

s{4} = find(vertex_lut(:,1)>= x & vertex_lut(:,2) >=-y &...
     vertex_lut(:,2)<=y);%
 
s{5} = find(vertex_lut(:,1)>= x & vertex_lut(:,2) <=-y);%

s{6} = find(vertex_lut(:,1)>= -x & vertex_lut(:,1)<= x &...
     vertex_lut(:,2)<=-y);%
 
s{7} = find(vertex_lut(:,1)<= -x & vertex_lut(:,2)<=-y);%

s{8} = find(vertex_lut(:,1)<= -x & vertex_lut(:,2)<= y &...
     vertex_lut(:,2)>=-y);%
 
% s = [s{1};s{2};s{3};s{4};s{5};s{6};s{7};s{8};s{9}]; 

%计算角区s1 s3 s5 s7 的Z值
for i= 1:4
    corner    = inter_corner(i,:);   
    deep_lut  = vertex_lut(s{2*i-1},1:2);
    size_area = size(deep_lut);
    for j=1:size_area(1,1)
        rd2=(deep_lut(j,1)-corner(1))^2 + (deep_lut(j,2)-corner(2))^2;
        rd = sqrt(rd2);
        if rd < dist
            deep_lut(j,3) =0;
        else
            deep_lut(j,3) =0.55 *(rd-dist)^2;%z =f(rd)  =k*rd^2
        end
    end
    vertex_lut(s{2*i-1},1:3) = deep_lut;
    deep_lut =[];%释放数据
end

% scatter3(vertex_lut(:,1),vertex_lut(:,2),vertex_lut(:,3),'k');

%计算变区s2 s4 s6 s8的Z值
weight = [0  1 -1  0;...
          1  0  0 -1;...
          0  1  1  0;...
          1  0  0  1];
     
 for i=1:4
    deep_lut = vertex_lut(s{2*i},1:2);
    size_area = size(deep_lut);
    for j=1:size_area(1,1)
        rd = abs([deep_lut(j,1:2),y,x]*weight(i,:)');%[x,y,h/2,w/2]
        if rd < dist
             deep_lut(j,3) =0;
        else
             deep_lut(j,3) =0.55*(rd-dist)^2;
        end
    end
    vertex_lut(s{2*i},1:3) = deep_lut;
    deep_lut =[];%释放数据
 end
 
 %计算区域s9的Z值，为零
 
 %数值截取
 vertex_lut(vertex_lut(:,3)>1000000,3)=1000000;
 vertex_pints = vertex_lut(:,1:3);
 
 scatter3(vertex_lut(:,1),vertex_lut(:,2),vertex_lut(:,3),'k');
 
 disp("finished lut calculate");
 
 