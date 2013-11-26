#ad_keyword
==========
###多关键字匹配（最小支持一个中文或2两个英文数字匹配）  
**Config**  
ad_keword.pattern 关键子所在文件路径（每个词组一行）   
**Functions**  
array ad_keywords (string $str [,int $type = AD_PATTERN_UNIQUE ] )    
$str  需要匹配的内容  
$type 返回匹配关键字   
  AD_PATTERN_UNIQUE 返回去重关键字  
  AD_PATTERN_ALL 返回所有关键字  



