library(Rcpp)
module = Rcpp::sourceCpp("r-joycons-udpread.cpp")
module

mag = function(a,b,c) sqrt(a*a + b*b + c*c)


for(j in 1:3)
for(i in 1:222) { 
  
  timer_start();
  x <- udp_message(); 
  
  dt = timer_elapsed();
  cat(i, dt*1000, x) 
  
  if(dt*1000>10) break();
}




time1 = system.time( m <- plyr::ldply(lapply(1:1500 , function(i) { read.table(textConnection(udp_message() ))  } )  )  )

#### filter signals ####
filt = function(x) filter(LP,x)

LP = butter(3,0.09,"low")
LP = cheby1(3,0.1,0.01,"low")
#LP <- ellip(3, 4, 19, 0.1)

n=150

LP <- fir1(290, 0.01)
#LP = blackman(n)

s1=which(LP==max(LP))[1]
s1
LP = LP[s1:length(LP)]

#LP=rev(LP)
LP = LP/sum(LP)
class(LP)<-"Ma"




filt = function(x) { x = fftfilt(LP,x);x[1:length(LP)]=x[length(LP)+1]; x;}


s = c(rep(0,100),rep(1,100))
plot( filt(s),t="l")
lines(s,col="blue")


#### plots ####
x=m[,1]
y=m[,2]
z=m[,3]

fx=filt(x);fy=filt(y);fz=filt(z)

ax = atan2(sqrt(y^2+z^2),x);
ay = atan2(sqrt(z^2+x^2),y);
az = atan2(sqrt(x^2+y^2),z);

fax = atan2(sqrt(fy^2+fz^2),fx);
fay = atan2(sqrt(fz^2+fx^2),fy);
faz = atan2(sqrt(fx^2+fy^2),fz);



plot(fax*360/3.14,t="l")
lines(ax*360/3.14,t="l",col="gray")

#plot(fay*360/3.14,t="l")
#plot(faz*360/3.14,t="l")
#lines(az*360/3.14,t="l",col="gray")


##### a3 #####
library(signal)
s =filt(diff(ay))

plot(s,t="l")
plot(s,t="l")

plot(ay[1:500],t="l")


#cal = apply(m,2,mean)
#earth = mean(mag(m[,1],m[,2],m[,3]))

plot(m[,5],t="l")
bias = mean(m[,5][1:150])
bias

s=100:1000
plot(m[s,5]-bias,t="l")

# 70 mdps / LSB

sum(m[s,5]) * 70 * (5/1000)

sum(m[s,5]) * 0.0027777778



plot( mag(m[,1],m[,2],m[,3]) , t="l",ylim=c(0,4000))

plot(m[,4],t="l")
plot(m[,5],t="l")
plot(m[,6],t="l")


vec = m[,5] - cal[5]
plot(vec,t="l") #, ylim=c(-4000,4000))

max(runsum(vec))/180

plot( runsum(vec)/1106 )

#avg=median(m[,1])

vec = vec-avg

runsum = function(vec) { z=vec; for(i in 2:length(z)) { z[i] = z[i-1]+vec[i] }; return(z); }

m[,3]
plot(runsum(vec+900),t="l")
avg






plot(z,t="l")


bias = 0.92
alpha = 0
beta = 0
gamma = 0
Math.PI = 2*acos(0)

filt = function(acclx,accly, acclz, gyrox,gyroy, gyroz) {
  dt = 1/100
  
  norm = sqrt(acclx ** 2 + accly ** 2 + acclz ** 2);
  
  scale = Math.PI / 2;
  
  alpha <<- alpha + gyroz * dt;
  beta <<- bias * (beta + gyrox * dt) + (1.0 - bias) * (acclx * scale / norm);
  gamma <<- bias * (gamma + gyroy * dt) + (1.0 - bias) * (accly * -scale / norm); 
  
  cat(sprintf("%7.3f %7.3f %7.3f\n", alpha,beta,gamma ));
  
}

2000 deg per unit

deg_to_rad = (2*Math.PI/360)


for(i in 1:2200) {
  x = read.table(textConnection(udp_message() ) )
  rotconstant = 2000 * 
  #cat(mag(x[1,1],x[1,2],x[1,3]),"\n")
  
  filt(x[1,1]/earth, x[1,2]/earth, x[1,3]/earth,  x[1,4] * rotconstant,x[1,5] * rotconstant,x[1,6] * rotconstant )
  
}






