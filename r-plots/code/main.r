library(ggplot2)
library("reshape2")
library(grid)
library(scales)

setwd("~/projects/ir/seq_kb_ir/r-plots")

###########################################################################################################

U = read.csv("data/data.csv", header = FALSE)
t = seq(0,1,length=10)

row.names(U) <- NULL 

###########################################################################################################
###########################################################################################################
U = read.csv("data/thresholds.csv", header = TRUE)
tnew = seq(0,0.4,length=10)

smooth <- function(t,y,tnew)
{
  spanvalue = 0.3
  f <- loess(y~t)
  ysmooth <- predict(f)
  # ysmooth = y
  return (ysmooth)
}

y1 = smooth(U$th, U$TREC7n8.beta_U)
y1 = y1 + 0.27 - max(y1)
y2 = smooth(U$th, U$TREC7n8.beta_L)
y2 = y2 + 0.27 - max(y2)

df = rbind(
  data.frame(Threshold = "beta_U", x=U$th, y=y1),
  data.frame(Threshold = 'beta_L', x=U$th, y=y2)
)

p <- ggplot(data = df, aes(x=x, y=y, color = Threshold, shape = Threshold)) +
  # geom_smooth(se=FALSE)+
  geom_line()+
  geom_point()+
  theme(legend.position = c(0.85, .35),
        # theme(legend.position = c(0.6, .45),
        legend.background = element_rect(fill=alpha('white', 0.1)))+
  # guides(col = guide_legend(ncol = 3))+
  scale_color_manual(name = "",
                     labels = c(expression(beta[U]), expression(beta[L]), expression(alpha[1])),
                     values = c("black", "blue", "red"))+
  scale_shape_manual(name = "",
                     labels = c(expression(beta[U]), expression(beta[L]), expression(alpha[1])),
                     values = c(0, 1, 2))+
  theme(legend.position = c(0.2, 0.25),
        # theme(legend.position = c(0.6, .45),
        legend.background = element_rect(fill=alpha('white', 0.1)))+
  xlab("Threshold")+
  ylab("MAP")+ 
  labs(color="")+
  labs(shape="")

print(p)
ggsave("images/TREC7n8.pdf", width = 2.75, height = 1.8)


###########################################################################################################
U = read.csv("data/thresholds.csv", header = TRUE)
tnew = seq(0,0.4,length=10)

smooth <- function(t,y,tnew)
{
  spanvalue = 0.3
  f <- loess(y~t)
  ysmooth <- predict(f)
  # ysmooth = y
  return (ysmooth)
}


y1 = smooth(U$th,U$ROBUST04.beta_U,tnew)
y2 = smooth(U$th,U$ROBUST04.beta_L,tnew)
y2 = y2 + max(y1) - max(y2)


df = rbind(
  data.frame(Threshold = "beta_U", x=U$th, y=y1),
  data.frame(Threshold = 'beta_L', x=U$th, y=y2)
)

p <- ggplot(data = df, aes(x=x, y=y, color = Threshold, shape = Threshold)) +
  geom_line()+
  geom_point()+
  theme(legend.position = c(0.85, .35),
        # theme(legend.position = c(0.6, .45),
        legend.background = element_rect(fill=alpha('white', 0.1)))+
  # guides(col = guide_legend(ncol = 3))+
  scale_color_manual(name = "",
                     labels = c(expression(beta[U]), expression(beta[L]), expression(alpha[1])),
                     values = c("black", "blue", "red"))+
  scale_shape_manual(name = "",
                     labels = c(expression(beta[U]), expression(beta[L]), expression(alpha[1])),
                     values = c(0, 1, 2))+
  theme(legend.position = c(0.2, 0.25),
        # theme(legend.position = c(0.6, .45),
        legend.background = element_rect(fill=alpha('white', 0.1)))+
  xlab("Threshold")+
  ylab("MAP")+ 
  labs(color="")+
  labs(shape="")

print(p)
ggsave("images/ROBUST04.pdf", width = 2.75, height = 1.8)

###########################################################################################################
U = read.csv("data/thresholds.csv", header = TRUE)
tnew = seq(0,0.4,length=10)

smooth <- function(t,y,tnew)
{
  spanvalue = 0.3
  f <- loess(y~t)
  ysmooth <- predict(f)
  # ysmooth = y
  return (ysmooth)
}


y1 = smooth(U$th,U$GOV.beta_U,tnew)
y2 = smooth(U$th,U$GOV.beta_L,tnew)
y2 = y2 + max(y1) - max(y2)


df = rbind(
  data.frame(Threshold = "beta_U", x=U$th, y=y1),
  data.frame(Threshold = 'beta_L', x=U$th, y=y2)
)

p <- ggplot(data = df, aes(x=x, y=y, color = Threshold, shape = Threshold)) +
  geom_line()+
  geom_point()+
  theme(legend.position = c(0.85, .35),
        # theme(legend.position = c(0.6, .45),
        legend.background = element_rect(fill=alpha('white', 0.1)))+
  # guides(col = guide_legend(ncol = 3))+
  scale_color_manual(name = "",
                     labels = c(expression(beta[U]), expression(beta[L]), expression(alpha[1])),
                     values = c("black", "blue", "red"))+
  scale_shape_manual(name = "",
                     labels = c(expression(beta[U]), expression(beta[L]), expression(alpha[1])),
                     values = c(0, 1, 2))+
  theme(legend.position = c(0.2, 0.25),
        # theme(legend.position = c(0.6, .45),
        legend.background = element_rect(fill=alpha('white', 0.1)))+
  xlab("Threshold")+
  ylab("MAP")+ 
  labs(color="")+
  labs(shape="")

print(p)
ggsave("images/GOV.pdf", width = 2.75, height = 1.8)

###########################################################################################################

U = read.csv("data.csv", header = FALSE)
t = seq(0,1,length=10)
tnew = seq(0,0.4,length=10)

smooth <- function(t,U,ind,tnew)
{
  spanvalue = 0.5
  f <- loess(as.numeric(unlist(U[ind,1:length(t)]))~t, span = spanvalue)
  ysmooth <- predict(f, newdata=tnew)
  return (ysmooth)
}

y11 = smooth(t,U,24,tnew)
y11 = y11 + 0.27 - max(y11)
y12 = smooth(t,U,6,tnew)
y12 = y12 + 0.27 - max(y12)

y21 = smooth(t,U,25,tnew)
y22 = smooth(t,U,7,tnew)
y22 = y22 + max(y21) - max(y22)

y31 = smooth(t,U,26,tnew)
y32 = smooth(t,U,8,tnew)
y32 = y32 + max(y31) - max(y32)


df = rbind(
  data.frame(Parameter = "beta_U", Collection = "TREC7n8",  x=tnew, y=y11),
  data.frame(Parameter = 'beta_L', Collection = "TREC7n8",  x=tnew, y=y12),
  data.frame(Parameter = "beta_U", Collection = "ROBUST04", x=tnew, y=y21),
  data.frame(Parameter = 'beta_L', Collection = "ROBUST04", x=tnew, y=y22),
  data.frame(Parameter = "beta_U", Collection = "GOV",      x=tnew, y=y31),
  data.frame(Parameter = 'beta_L', Collection = "GOV",      x=tnew, y=y32)
)
df$grp = paste(df$Collection,df$Parameter)

p <- ggplot(data = df, aes(x=x, y=y, color = grp, shape = grp, linetype = grp)) +
  # , group=interaction("Collection", "Parameter")
  geom_line()+
  geom_point(size=2)+
#   theme(legend.position = c(0.85, .35),
#         # theme(legend.position = c(0.6, .45),
#         legend.background = element_rect(fill=alpha('white', 0.1)))+
  # guides(col = guide_legend(ncol = 3))+
  scale_linetype_manual(name = "",
                      labels = c(
                                 expression(paste("GOV ", beta[U])), expression(paste("GOV ", beta[L])),
                                 expression(paste("ROBUST04 ", beta[U])), expression(paste("ROBUST04 ", beta[L])),
                                 expression(paste("TREC7-8 ", beta[U])), expression(paste("TREC7-8 ", beta[L]))
                                 ),
                      values = c(1, 2, 1, 2, 1, 2))+
  scale_color_manual(name = "",
                     labels = c(
                       expression(paste("GOV ", beta[U])), expression(paste("GOV ", beta[L])),
                       expression(paste("ROBUST04 ", beta[U])), expression(paste("ROBUST04 ", beta[L])),
                       expression(paste("TREC7-8 ", beta[U])), expression(paste("TREC7-8 ", beta[L]))
                     ),
                     values = c("black", "black", "blue", "blue", "red", "red"))+
  scale_shape_manual(name = "",
                     labels = c(
                       expression(paste("GOV ", beta[U])), expression(paste("GOV ", beta[L])),
                       expression(paste("ROBUST04 ", beta[U])), expression(paste("ROBUST04 ", beta[L])),
                       expression(paste("TREC7-8 ", beta[U])), expression(paste("TREC7-8 ", beta[L]))
                     ),
                     values = c(0, 0, 1, 1, 2, 2))+
#   theme(legend.position = c(0.8, 0.8),
#         # theme(legend.position = c(0.6, .45),
#         legend.background = element_rect(fill=alpha('white', 0.1)))+
  xlab("Parameter")+
  ylab("MAP")+ 
  labs(color="")+
  labs(shape="")

print(p)
ggsave("images/thresholds.pdf", width = 4.15, height = 3.24)

###########################################################################################################
df = rbind(
data.frame(Collection = "TREC7-8", x=t, y=as.numeric(unlist(U[24,1:length(t)]))),
data.frame(Collection = "ROBUST04", x=t, y=as.numeric(unlist(U[25,1:length(t)]))),
data.frame(Collection = "GOV", x=t, y=as.numeric(unlist(U[26,1:length(t)])))
)

p <- ggplot(data = df, aes(x=x, y=y, color = Collection, shape = Collection)) +
  geom_line()+
  geom_point(size=2)+
  theme(legend.position = c(0.85, .35),
  # theme(legend.position = c(0.6, .45),
        legend.background = element_rect(fill=alpha('white', 0.1)))+
  # guides(col = guide_legend(ncol = 3))+
  xlab(expression(beta[U]))+
  ylab("MAP")+ 
  labs(color="")+
  labs(shape="")

print(p)
ggsave("images/beta_U.pdf")

###########################################################################################################
df = rbind(
  data.frame(Collection = "TREC7-8", x=t, y=as.numeric(unlist(U[6,1:length(t)]))),
  data.frame(Collection = "ROBUST04", x=t, y=as.numeric(unlist(U[7,1:length(t)]))),
  data.frame(Collection = "GOV", x=t, y=as.numeric(unlist(U[8,1:length(t)])))
)
p <- ggplot(data = df, aes(x=x, y=y, color = Collection, shape = Collection)) +
  geom_line()+
  geom_point(size=2)+
  theme(legend.position = c(0.85, .35),
        legend.background = element_rect(fill=alpha('white', 0.1)))+
  # guides(col = guide_legend(ncol = 3))+
  xlab(expression(beta[L]))+
  ylab("MAP")+ 
  labs(color="")+
  labs(shape="")

print(p)
ggsave("images/beta_L.pdf")

###########################################################################################################
df = rbind(
  data.frame(Collection = "TREC7-8", x=t, y=as.numeric(unlist(U[19,1:length(t)]))),
  data.frame(Collection = "ROBUST04", x=t, y=as.numeric(unlist(U[20,1:length(t)]))),
  data.frame(Collection = "GOV", x=t, y=as.numeric(unlist(U[21,1:length(t)])))
)
p <- ggplot(data = df, aes(x=x, y=y, color = Collection, shape = Collection)) +
  geom_line()+
  geom_point(size=2)+
  theme(legend.position = c(0.85, .65),
        legend.background = element_rect(fill=alpha('white', 0.1)))+
#   theme(legend.position="top",
#         legend.background = element_rect(fill=alpha('white', 0.1)))+
  # guides(col = guide_legend(ncol = 3))+
  xlab(expression(alpha[1]))+
  ylab("MAP")+ 
  labs(color="")+
  labs(shape="")

print(p)
ggsave("images/alpha_1.pdf")

###########################################################################################################
###########################################################################################################

df = rbind(
  data.frame(Parameter = "beta_U", x=t, y=as.numeric(unlist(U[24,1:length(t)]))),
  data.frame(Parameter = 'beta_L', x=t, y=as.numeric(unlist(U[6,1:length(t)]))),
  data.frame(Parameter = "alpha_1", x=t, y=as.numeric(unlist(U[19,1:length(t)])))
)

p <- ggplot(data = df, aes(x=x, y=y, color = Parameter, shape = Parameter)) +
  geom_line()+
  geom_point(size=2)+
  theme(legend.position = c(0.85, .35),
        # theme(legend.position = c(0.6, .45),
        legend.background = element_rect(fill=alpha('white', 0.1)))+
  # guides(col = guide_legend(ncol = 3))+
  scale_color_manual(name = "Parameter",
                     labels = c(expression(beta[U]), expression(beta[L]), expression(alpha[1])),
                     values = c("black", "blue", "red"))+
  scale_shape_manual(name = "Parameter",
                     labels = c(expression(beta[U]), expression(beta[L]), expression(alpha[1])),
                     values = c(0, 1, 2))+
  theme(legend.position = c(0.85, 0.6),
        # theme(legend.position = c(0.6, .45),
        legend.background = element_rect(fill=alpha('white', 0.1)))+
  xlab("Parameter")+
  ylab("MAP")+ 
  labs(color="")+
  labs(shape="")

print(p)
ggsave("images/TREC7n8.pdf", width = 4.15, height = 3.24)

###########################################################################################################
df = rbind(
  data.frame(Parameter = "beta_U", x=t, y=as.numeric(unlist(U[25,1:length(t)]))),
  data.frame(Parameter = 'beta_L', x=t, y=as.numeric(unlist(U[7,1:length(t)]))),
  data.frame(Parameter = "alpha_1", x=t, y=as.numeric(unlist(U[20,1:length(t)])))
)

p <- ggplot(data = df, aes(x=x, y=y, color = Parameter, shape = Parameter)) +
  geom_line()+
  geom_point(size=2)+
  theme(legend.position = c(0.85, .35),
        # theme(legend.position = c(0.6, .45),
        legend.background = element_rect(fill=alpha('white', 0.1)))+
  # guides(col = guide_legend(ncol = 3))+
  scale_color_manual(name = "Parameter",
                     labels = c(expression(beta[U]), expression(beta[L]), expression(alpha[1])),
                     values = c("black", "blue", "red"))+
  scale_shape_manual(name = "Parameter",
                     labels = c(expression(beta[U]), expression(beta[L]), expression(alpha[1])),
                     values = c(0, 1, 2))+
  theme(legend.position = c(0.85, 0.6),
        # theme(legend.position = c(0.6, .45),
        legend.background = element_rect(fill=alpha('white', 0.1)))+
  xlab("Parameter")+
  ylab("MAP")+ 
  labs(color="")+
  labs(shape="")

print(p)
ggsave("images/ROBUST04.pdf", width = 4.15, height = 3.24)

###########################################################################################################
df = rbind(
  data.frame(Parameter = "beta_U", x=t, y=as.numeric(unlist(U[26,1:length(t)]))),
  data.frame(Parameter = 'beta_L', x=t, y=as.numeric(unlist(U[8,1:length(t)]))),
  data.frame(Parameter = "alpha_1", x=t, y=as.numeric(unlist(U[21,1:length(t)])))
)

p <- ggplot(data = df, aes(x=x, y=y, color = Parameter, shape = Parameter)) +
  geom_line()+
  geom_point(size=2)+
  theme(legend.position = c(0.85, .35),
        # theme(legend.position = c(0.6, .45),
        legend.background = element_rect(fill=alpha('white', 0.1)))+
  # guides(col = guide_legend(ncol = 3))+
  scale_color_manual(name = "Parameter",
                     labels = c(expression(beta[U]), expression(beta[L]), expression(alpha[1])),
                     values = c("black", "blue", "red"))+
  scale_shape_manual(name = "Parameter",
                     labels = c(expression(beta[U]), expression(beta[L]), expression(alpha[1])),
                     values = c(0, 1, 2))+
  theme(legend.position = c(0.2, 0.3),
        # theme(legend.position = c(0.6, .45),
        legend.background = element_rect(fill=alpha('white', 0.1)))+
  xlab("Parameter")+
  ylab("MAP")+ 
  labs(color="")+
  labs(shape="")

print(p)
ggsave("images/GOV.eps", width = 4.15, height = 3.24)

###########################################################################################################

#x = cbind(matrix(1,100,1), as.matrix(x))

setEPS()
postscript("beta_U.eps")

plot(t, U[24,1:length(t)], col="blue", type="o", ylim=c(0.19,0.3), xlab=expression(beta[U]), ylab='MAP', cex=1.7, pch =8, cex.lab = 1.7, cex.axis=1.5)

lines(t, U[25,1:length(t)], col="black", type="o", pch = 3)
lines(t, U[26,1:length(t)], col="red", type="o", pch = 2);

grid()
legend("bottomleft", c("TREC 7-8", "ROBUST04", "GOV"), lty=c(1, 1, 1), lwd=c(2.5, 2.5), 
       col=c("blue", "black", "red"),pch = c(8, 3, 2), cex=1.7) 
dev.off()

###########################################################################################################

setEPS()
postscript("beta_L.eps")

t = seq(0,0.5,length=10)

#line(U[,2], type="p",col="blue", pch=0, xlab="x", ylab="y")
#line(U[,2], type="p",col="blue", pch=0, xlab="x", ylab="y")
plot(t, U[6,1:length(t)], col="blue", type="o", ylim=c(0.19,0.3), xlab=expression(beta[L]), ylab='MAP', cex=1.7, pch =8, cex.lab = 1.7, cex.axis=1.5)

lines(t, U[7,1:length(t)], col="black", type="o", pch = 3)
lines(t, U[8,1:length(t)], col="red", type="o", pch = 2);

grid()
legend("bottomleft", c("TREC 7-8", "ROBUST04", "GOV"), lty=c(1, 1, 1), lwd=c(2.5, 2.5), 
       col=c("blue", "black", "red"),pch = c(8, 3, 2), cex=1.7) 
dev.off()

###########################################################################################################

setEPS()
postscript("alpha.eps")

t = seq(100,3500,length=18)

plot(t, U[19,1:length(t)], col="blue", type="o", ylim=c(0.1,0.35), xlab=expression(alpha[1]), ylab='MAP', cex=1.7, pch =8, cex.lab = 1.7, cex.axis=1.5)

lines(t, U[20,1:length(t)], col="black", type="o", pch = 3)
lines(t, U[21,1:length(t)], col="red", type="o", pch = 2);

grid()
legend("bottomleft", c("TREC 7-8", "ROBUST04", "GOV"), lty=c(1, 1, 1), lwd=c(2.5, 2.5), 
       col=c("blue", "black", "red"),pch = c(8, 3, 2), cex=1.7) 
dev.off()



###########################################################################################################

illustrations <- read.csv("illustrations2.csv", header = TRUE, sep = ",", quote = "\"")

N = length(illustrations$Q)
data=data.frame(x=seq(1,N), y=illustrations$Q, l=illustrations$level, c=illustrations$concept, p=illustrations$pairs)

th = c(2.5,3.45,4.5,7.5,9.55,11.5,12.5,12.5+0.1)
th2 = c(0.5,3.55,9.45,12.5-0.1,15.5)

data$col = rep("black", N)
data$fill = data$l
cuts1 <- data.frame(Thresholds="Layer Thresholds", vals=th2[1:length(th2)])
cuts2 <- data.frame(Thresholds="Upper Thresholds", vals=c(th[1], th[3], th[5], th[7]))
cuts3 <- data.frame(Thresholds="Lower Thresholds", vals=c(th[2], th[4], th[6], th[8]))
cuts <- rbind(cuts1,cuts2,cuts3)

y_annotate = 0.287

p <- ggplot(data, aes(x, y, fill=factor(data$fill), width=.7))+
  geom_text(aes(label=p), vjust=-0.5, size=3)+
  geom_bar(color="black", stat="identity")+
  geom_vline(data=cuts, aes(xintercept = vals,
                    linetype=Thresholds,
                    colour = Thresholds))+
#   geom_segment(aes(xend=th[2], yend=0.255, x=th[1], y=0.255),
#                arrow=arrow(length=unit(0.2,"cm")))+
#   geom_segment(aes(xend=th[1], yend=0.255, x=th[2], y=0.255),
#                arrow=arrow(length=unit(0.2,"cm")))+
#   geom_segment(aes(xend=th[3], yend=0.255, x=th[4], y=0.255),
#                arrow=arrow(length=unit(0.2,"cm")))+
#   geom_segment(aes(xend=th[4], yend=0.255, x=th[3], y=0.255),
#                arrow=arrow(length=unit(0.2,"cm")))+
#   geom_segment(aes(xend=th[6]-0.1, yend=0.255, x=th[5], y=0.255),
#                arrow=arrow(length=unit(0.2,"cm")))+
#   geom_segment(aes(xend=th[5], yend=0.255, x=th[6]-0.1, y=0.255),
#                arrow=arrow(length=unit(0.2,"cm")))+
  annotate("text", label="Selection\nRegion",   x=(th2[1]+th[1])/2, y=y_annotate+0.0015, size=2.5)+
  annotate("text", label="Selection\nRegion",   x=(th2[2]+th[3])/2, y=y_annotate-0.01,   size=2.5, angle=90)+
  # annotate("text", label="Selection\nRegion",   x=(th2[3]+th[5])/2, y=y_annotate-0.01,   size=2.5, angle=90)+
  annotate("text", label="Uncertainty\nRegion", x=(th[1]+th[2])/2,  y=y_annotate-0.01,        size=2.5, angle=90)+
  annotate("text", label="Uncertainty\nRegion", x=(th[3]+th[4])/2,  y=y_annotate-0.01,   size=2.5)+
  annotate("text", label="Uncertainty\nRegion", x=(th[5]+th[6])/2,  y=y_annotate-0.01,   size=2.5, angle=0)+
  # annotate("text", label="Rejection\nRegion",   x=(th[2]+th2[2])/2, y=y_annotate,        size=2.5)+
  annotate("text", label="Rejection\nRegion",   x=(th[4]+th2[3])/2, y=y_annotate-0.01,   size=2.5, angle=0)+
  annotate("text", label="Rejection\nRegion",   x=(th[6]+th2[4])/2, y=y_annotate-0.01,   size=2.5, angle=90)+
  annotate("text", label="Rejection\nRegion",   x=(th[8]+th2[5])/2, y=y_annotate-0.01,   size=2.5)+
  # guides(fill = guide_legend(ncol = 4), linetype=guide_legend(ncol=3))+
  scale_fill_manual(name = "Concept Layers",
                    labels = c("Layer 1", "Layer 2", "Layer 3",
                               "Layer 4"),
                    values = c("black", "dimgray", "gray", 
                               "white"))+
  scale_linetype_manual(name = "Thresholds",
                    labels = c("Layer Thresholds", "Upper Thresholds", "Lower Thresholds"),
                    values = c("solid", "dashed", "dotted"))+
  scale_color_manual(name = "Thresholds",
                        labels = c("Layer Thresholds", "Upper Thresholds", "Lower Thresholds"),
                        values = c("black", "blue", "red"))+
   # guides(fill = guide_legend(keywidth = 1, keyheight = 0.85, ncol = 1))+
#          linetype = guide_legend(keywidth = 1, keyheight = 1, ncol = 1))+
  scale_x_continuous(breaks = seq(1,N), labels = seq(1,N))+
  xlab("Concepts")+
  ylab(expression(Q[s](c)))+
  scale_y_continuous(limits=c(min(data$y), max(data$y)*1.1), oob = scales::rescale_none)+
  # theme(legend.position = c(0.6, .87), legend.margin = unit(0.01, "cm"),legend.key.size = unit(0.5, "cm"))
  theme(legend.margin = unit(0.01, "cm"),legend.key.size = unit(0.4, "cm"))

print(p)
ggsave("images/SNCPO.pdf", width = 7.29, height = 1.97)
# Saving 7.29 x 1.97 in image
###########################################################################################################

illustrations <- read.csv("illustrations2.csv", header = TRUE, sep = ",", quote = "\"")

N = length(illustrations$Q)
data=data.frame(x=seq(1,N), y=illustrations$Q, l=illustrations$level, c=illustrations$concept, p=illustrations$pairs)

th = c(0.252,0.205,0.238,0.208,0.232,0.215,0.227,0.219)
th2 = c(0.5,3.5,9.5,12.5,15.5)
thr = c(2.5,5.5,6.5,10.5,11.5)

data$col = rep("black", N)
data$fill = data$l
cuts1 <- data.frame(Thresholds="Layer Thresholds", vals=th2[1:length(th2)])
cuts2 <- data.frame(Thresholds="Upper Thresholds", vals=c(th[1], th[3], th[5], th[7]))
cuts3 <- data.frame(Thresholds="Lower Thresholds", vals=c(th[2], th[4], th[6], th[8]))
cuts <- rbind(cuts2,cuts3)
  
  threshold1 <-                   data.frame(Thresholds="Lower Thresholds", xend1=th2[2], yend1=th[1], x1=th2[1], y1=th[1], color1="blue", linetype1="dash")
  threshold1 <- rbind(threshold1, data.frame(Thresholds="Upper Thresholds", xend1=th2[2], yend1=th[2], x1=th2[1], y1=th[2], color1="red",  linetype1="dotted"))
  threshold1 <- rbind(threshold1, data.frame(Thresholds="Lower Thresholds", xend1=th2[3], yend1=th[3], x1=th2[2], y1=th[3], color1="blue", linetype1="dash"))
  threshold1 <- rbind(threshold1, data.frame(Thresholds="Upper Thresholds", xend1=th2[3], yend1=th[4], x1=th2[2], y1=th[4], color1="red",  linetype1="dotted"))
  threshold1 <- rbind(threshold1, data.frame(Thresholds="Lower Thresholds", xend1=th2[4], yend1=th[5], x1=th2[3], y1=th[5], color1="blue", linetype1="dash"))
  threshold1 <- rbind(threshold1, data.frame(Thresholds="Upper Thresholds", xend1=th2[4], yend1=th[6], x1=th2[3], y1=th[6], color1="red",  linetype1="dotted"))
  threshold1 <- rbind(threshold1, data.frame(Thresholds="Lower Thresholds", xend1=th2[5], yend1=th[7], x1=th2[4], y1=th[7], color1="blue", linetype1="dash"))
  threshold1 <- rbind(threshold1, data.frame(Thresholds="Upper Thresholds", xend1=th2[5], yend1=th[8], x1=th2[4], y1=th[8], color1="red",  linetype1="dotted"))
  
  yR_max = 0.284
  
  threshold2 <-                   data.frame(Thresholds="Selection Region",   x1=th2[1]+0.05, y1=yR_max+0.001,   x2=thr[1]-0.05, y2=yR_max+0.001)
#   threshold2 <- rbind(threshold2, data.frame(Thresholds="Uncertainty Region", x1=thr[1]+0.05, y1=yR_max-0.018,   x2=thr[2]-0.05, y2=yR_max-0.018))
  threshold2 <- rbind(threshold2, data.frame(Thresholds="Rejection Region",   x1=thr[1]+0.05, y1=yR_max-0.018,   x2=th2[2]-0.05, y2=yR_max-0.018))
  threshold2 <- rbind(threshold2, data.frame(Thresholds="Selection Region",   x1=th2[2]+0.05, y1=yR_max-0.018,   x2=thr[2]-0.05, y2=yR_max-0.018))
  threshold2 <- rbind(threshold2, data.frame(Thresholds="Uncertainty Region", x1=thr[2]+0.05, y1=yR_max-0.018,   x2=thr[3]-0.05, y2=yR_max-0.018))
  threshold2 <- rbind(threshold2, data.frame(Thresholds="Rejection Region",   x1=thr[3]+0.05, y1=yR_max-0.018,   x2=th2[3]-0.05, y2=yR_max-0.018))
  threshold2 <- rbind(threshold2, data.frame(Thresholds="Selection Region",   x1=th2[3]+0.05, y1=yR_max-0.018,   x2=thr[4]-0.05, y2=yR_max-0.018))
  threshold2 <- rbind(threshold2, data.frame(Thresholds="Uncertainty Region", x1=thr[4]+0.05, y1=yR_max-0.018,   x2=thr[5]-0.05, y2=yR_max-0.018))
  threshold2 <- rbind(threshold2, data.frame(Thresholds="Rejection Region",   x1=thr[5]+0.05, y1=yR_max-0.018,   x2=th2[4]-0.05, y2=yR_max-0.018))
  threshold2 <- rbind(threshold2, data.frame(Thresholds="Rejection Region",   x1=th2[4]+0.05, y1=yR_max-0.018,   x2=th2[5]-0.05, y2=yR_max-0.018))
  
  y_annotate = 0.292
  
    p <- ggplot(data, aes(x, y, fill=factor(fill), width=.7))+
    annotate("text", label="Selection\nRegion",   x=(th2[1]+thr[1])/2,  y=y_annotate+0.0015, size=2.5)+
#     annotate("text", label="Uncertainty\nRegion", x=(thr[1]+thr[2])/2,  y=y_annotate-0.01,  size=2.5)+
    annotate("text", label="Rejection\nRegion",   x=(thr[1]+th2[2])/2,  y=y_annotate-0.008,  size=2.5, angle=90)+
    annotate("text", label="Selection\nRegion",   x=(th2[2]+thr[2])/2,  y=y_annotate-0.008,  size=2.5, angle=0)+
    annotate("text", label="Uncertainty\nRegion", x=(thr[2]+thr[3])/2,  y=y_annotate-0.008,  size=2.5, angle=90)+
    annotate("text", label="Rejection\nRegion",   x=(thr[3]+th2[3])/2,  y=y_annotate-0.008,  size=2.5, angle=0)+
    annotate("text", label="Selection\nRegion",   x=(th2[3]+thr[4])/2,  y=y_annotate-0.008,  size=2.5, angle=90)+
    annotate("text", label="Uncertainty\nRegion", x=(thr[4]+thr[5])/2,  y=y_annotate-0.008,  size=2.5, angle=90)+
    annotate("text", label="Rejection\nRegion",   x=(thr[5]+th2[4])/2,  y=y_annotate-0.008,  size=2.5, angle=90)+
    annotate("text", label="Rejection\nRegion",   x=(th2[4]+th2[5])/2,  y=y_annotate-0.01,  size=2.5)+
    geom_bar(color="black", stat="identity")+
    geom_segment(data = threshold1, aes(xend=xend1, yend=yend1, x=x1, y=y1, 
                                        fill=NA, color=Thresholds,
                                        linetype=Thresholds), show.legend=F)+
    geom_segment(data = threshold2, aes(xend=x1, yend=y1, x=x2, y=y2, 
                                        fill=NA), show.legend=F, size=0.25,
                                        arrow=arrow(length=unit(0.15,"cm")))+
    geom_segment(data = threshold2, aes(xend=x2, yend=y2, x=x1, y=y1, 
                                        fill=NA), show.legend=F, size=0.25,
                                        arrow=arrow(length=unit(0.15,"cm")))+
    geom_text(aes(label=p), vjust=-0.5, size=3)+
    geom_vline(data=cuts1, aes(xintercept = vals,
                               linetype=Thresholds,
                              color=Thresholds))+
    scale_linetype_manual(name = "Thresholds",
                          labels = c("Layer Thresholds", "Upper Thresholds", "Lower Thresholds"),
                          values = c("solid", "dashed", "dotted"))+
    scale_color_manual(name = "Thresholds",
                       labels = c("Layer Thresholds", "Upper Thresholds", "Lower Thresholds"),
                       values = c("black", "blue", "red"))+
    scale_fill_manual(name = "Concept Layers",
                      labels = c("Layer 1", "Layer 2", "Layer 3",
                                 "Layer 4"),
                      values = c("black", "dimgray", "gray", 
                                 "white"))+
    scale_x_continuous(breaks = seq(1,N), labels = seq(1,N))+
    xlab("concepts")+
    ylab(expression(Q[r](c)))+
    scale_y_continuous(limits=c(min(data$y), max(data$y)*1.1), oob = scales::rescale_none)+
    theme(legend.margin = unit(0.01, "cm"),legend.key.size = unit(0.4, "cm"))
  
  print(p)
  ggsave("images/SNOPC.pdf", width = 7.29, height = 1.97)
# Saving 7.29 x 1.97 in image

  
###########################################################################################################

illustrations <- read.csv("data/illustrations_sorted_layers3.csv", header = TRUE, sep = ",", quote = "\"")

N = length(illustrations$Q)
data=data.frame(x=seq(1,N), y=illustrations$Q, l=illustrations$level, c=illustrations$concept, p=illustrations$pairs)

th = c(0.252,0.205,0.238,0.208,0.232,0.215,0.227,0.219)
th2 = c(0.5,3.5,9.5,12.5,15.5)
thr = c(2.5,5.5,6.5,10.5,11.5)

data$col = rep("black", N)
data$fill = data$l
cuts1 <- data.frame(Thresholds="Layer Thresholds", vals=th2[1:length(th2)])
cuts2 <- data.frame(Thresholds="Upper Thresholds", vals=c(th[1], th[3], th[5], th[7]))
cuts3 <- data.frame(Thresholds="Lower Thresholds", vals=c(th[2], th[4], th[6], th[8]))
cuts <- rbind(cuts2,cuts3)

threshold1 <-                   data.frame(Thresholds="Lower Thresholds", xend1=th2[2], yend1=th[1], x1=th2[1], y1=th[1], color1="blue", linetype1="dash")
threshold1 <- rbind(threshold1, data.frame(Thresholds="Upper Thresholds", xend1=th2[2], yend1=th[2], x1=th2[1], y1=th[2], color1="red",  linetype1="dotted"))
threshold1 <- rbind(threshold1, data.frame(Thresholds="Lower Thresholds", xend1=th2[3], yend1=th[3], x1=th2[2], y1=th[3], color1="blue", linetype1="dash"))
threshold1 <- rbind(threshold1, data.frame(Thresholds="Upper Thresholds", xend1=th2[3], yend1=th[4], x1=th2[2], y1=th[4], color1="red",  linetype1="dotted"))
threshold1 <- rbind(threshold1, data.frame(Thresholds="Lower Thresholds", xend1=th2[4], yend1=th[5], x1=th2[3], y1=th[5], color1="blue", linetype1="dash"))
threshold1 <- rbind(threshold1, data.frame(Thresholds="Upper Thresholds", xend1=th2[4], yend1=th[6], x1=th2[3], y1=th[6], color1="red",  linetype1="dotted"))
threshold1 <- rbind(threshold1, data.frame(Thresholds="Lower Thresholds", xend1=th2[5], yend1=th[7], x1=th2[4], y1=th[7], color1="blue", linetype1="dash"))
threshold1 <- rbind(threshold1, data.frame(Thresholds="Upper Thresholds", xend1=th2[5], yend1=th[8], x1=th2[4], y1=th[8], color1="red",  linetype1="dotted"))

yR_max = 0.284

threshold2 <-                   data.frame(Thresholds="Selection Region",   x1=th2[1]+0.05, y1=yR_max+0.001,   x2=thr[1]-0.05, y2=yR_max+0.001)
#   threshold2 <- rbind(threshold2, data.frame(Thresholds="Uncertainty Region", x1=thr[1]+0.05, y1=yR_max-0.018,   x2=thr[2]-0.05, y2=yR_max-0.018))
threshold2 <- rbind(threshold2, data.frame(Thresholds="Rejection Region",   x1=thr[1]+0.05, y1=yR_max-0.018,   x2=th2[2]-0.05, y2=yR_max-0.018))
threshold2 <- rbind(threshold2, data.frame(Thresholds="Selection Region",   x1=th2[2]+0.05, y1=yR_max-0.018,   x2=thr[2]-0.05, y2=yR_max-0.018))
threshold2 <- rbind(threshold2, data.frame(Thresholds="Uncertainty Region", x1=thr[2]+0.05, y1=yR_max-0.018,   x2=thr[3]-0.05, y2=yR_max-0.018))
threshold2 <- rbind(threshold2, data.frame(Thresholds="Rejection Region",   x1=thr[3]+0.05, y1=yR_max-0.018,   x2=th2[3]-0.05, y2=yR_max-0.018))
threshold2 <- rbind(threshold2, data.frame(Thresholds="Selection Region",   x1=th2[3]+0.05, y1=yR_max-0.018,   x2=thr[4]-0.05, y2=yR_max-0.018))
threshold2 <- rbind(threshold2, data.frame(Thresholds="Uncertainty Region", x1=thr[4]+0.05, y1=yR_max-0.018,   x2=thr[5]-0.05, y2=yR_max-0.018))
threshold2 <- rbind(threshold2, data.frame(Thresholds="Rejection Region",   x1=thr[5]+0.05, y1=yR_max-0.018,   x2=th2[4]-0.05, y2=yR_max-0.018))
threshold2 <- rbind(threshold2, data.frame(Thresholds="Rejection Region",   x1=th2[4]+0.05, y1=yR_max-0.018,   x2=th2[5]-0.05, y2=yR_max-0.018))

y_annotate = 0.292

p <- ggplot(data, aes(x, y, fill=factor(fill), width=.7))+
#   annotate("text", label="Selection\nRegion",   x=(th2[1]+thr[1])/2,  y=y_annotate+0.0015, size=2.5)+
#   #     annotate("text", label="Uncertainty\nRegion", x=(thr[1]+thr[2])/2,  y=y_annotate-0.01,  size=2.5)+
#   annotate("text", label="Rejection\nRegion",   x=(thr[1]+th2[2])/2,  y=y_annotate-0.008,  size=2.5, angle=90)+
#   annotate("text", label="Selection\nRegion",   x=(th2[2]+thr[2])/2,  y=y_annotate-0.008,  size=2.5, angle=0)+
#   annotate("text", label="Uncertainty\nRegion", x=(thr[2]+thr[3])/2,  y=y_annotate-0.008,  size=2.5, angle=90)+
#   annotate("text", label="Rejection\nRegion",   x=(thr[3]+th2[3])/2,  y=y_annotate-0.008,  size=2.5, angle=0)+
#   annotate("text", label="Selection\nRegion",   x=(th2[3]+thr[4])/2,  y=y_annotate-0.008,  size=2.5, angle=90)+
#   annotate("text", label="Uncertainty\nRegion", x=(thr[4]+thr[5])/2,  y=y_annotate-0.008,  size=2.5, angle=90)+
#   annotate("text", label="Rejection\nRegion",   x=(thr[5]+th2[4])/2,  y=y_annotate-0.008,  size=2.5, angle=90)+
#   annotate("text", label="Rejection\nRegion",   x=(th2[4]+th2[5])/2,  y=y_annotate-0.01,  size=2.5)+
  geom_bar(color="black", stat="identity")+
#   geom_segment(data = threshold1, aes(xend=xend1, yend=yend1, x=x1, y=y1, 
#                                       fill=NA, color=Thresholds,
#                                       linetype=Thresholds), show.legend=F)+
#   geom_segment(data = threshold2, aes(xend=x1, yend=y1, x=x2, y=y2, 
#                                       fill=NA), show.legend=F, size=0.25,
#                arrow=arrow(length=unit(0.15,"cm")))+
#   geom_segment(data = threshold2, aes(xend=x2, yend=y2, x=x1, y=y1, 
#                                       fill=NA), show.legend=F, size=0.25,
#                arrow=arrow(length=unit(0.15,"cm")))+
  geom_text(aes(label=p), vjust=-0.5, size=3)+
  geom_vline(data=cuts1, aes(xintercept = vals,
                             linetype=Thresholds,
                             color=Thresholds))+
#   scale_linetype_manual(name = "Thresholds",
#                         labels = c("Layer Thresholds", "Upper Thresholds", "Lower Thresholds"),
#                         values = c("solid", "dashed", "dotted"))+
  scale_color_manual(name = "Thresholds",
                     labels = c("Layer Thresholds"),
                     values = c("black"))+
  scale_fill_manual(name = "Concept Layers",
                    labels = c("Layer 1", "Layer 2", "Layer 3",
                               "Layer 4"),
                    values = c("black", "dimgray", "gray", 
                               "white"))+
  scale_x_continuous(breaks = seq(1,N), labels = seq(1,N))+
  xlab("concepts")+
  ylab(expression(Q[s](c)))+
  scale_y_continuous(limits=c(min(data$y), max(data$y)*1.1), oob = scales::rescale_none)+
  theme(legend.margin = unit(0.01, "cm"),legend.key.size = unit(0.4, "cm"))

print(p)
ggsave("images/SNOPC_stepI.pdf", width = 7.29, height = 1.97)
# Saving 7.29 x 1.97 in image




