# compilerTiger

现代编译原理课后题答案
https://git.acwing.com/joykiller/tiger/-/tree/51ef5befbd21f3f0b9bae1a5f28939940ee29ec5/%E7%8E%B0%E4%BB%A3%E7%BC%96%E8%AF%91%E5%8E%9F%E7%90%86%E8%AF%BE%E5%90%8E%E9%A2%98%E7%AD%94%E6%A1%88



import React from 'react';


/*

使用下面描述的算法可以扰乱字符串 s 得到字符串 t ：
如果字符串的长度为 1 ，算法停止
如果字符串的长度 > 1 ，执行下述步骤：
在一个随机下标处将字符串分割成两个非空的子字符串。即，如果已知字符串 s ，则可以将其分成两个子字符串 x 和 y ，且满足 s = x + y 。
随机 决定是要「交换两个子字符串」还是要「保持这两个子字符串的顺序不变」。即，在执行这一步骤之后，s 可能是 s = x + y 或者 s = y + x 。
在 x 和 y 这两个子字符串上继续从步骤 1 开始递归执行此算法。
给你两个 长度相等 的字符串 s1 和 s2，判断 s2 是否是 s1 的扰乱字符串。如果是，返回 true ；否则，返回 false 。

 

示例 1：

输入：s1 = "great", s2 = "rgeat"
输出：true
解释：s1 上可能发生的一种情形是：
"great" --> "gr/eat" // 在一个随机下标处分割得到两个子字符串
"gr/eat" --> "gr/eat" // 随机决定：「保持这两个子字符串的顺序不变」
"gr/eat" --> "g/r / e/at" // 在子字符串上递归执行此算法。两个子字符串分别在随机下标处进行一轮分割
"g/r / e/at" --> "r/g / e/at" // 随机决定：第一组「交换两个子字符串」，第二组「保持这两个子字符串的顺序不变」
"r/g / e/at" --> "r/g / e/ a/t" // 继续递归执行此算法，将 "at" 分割得到 "a/t"
"r/g / e/ a/t" --> "r/g / e/ a/t" // 随机决定：「保持这两个子字符串的顺序不变」
算法终止，结果字符串和 s2 相同，都是 "rgeat"
这是一种能够扰乱 s1 得到 s2 的情形，可以认为 s2 是 s1 的扰乱字符串，返回 true
示例 2：

输入：s1 = "abcde", s2 = "caebd"
输出：false
示例 3：

输入：s1 = "a", s2 = "a"
输出：true

*/




/*
    1. 把 s2 换成 s1 的位置坐标， 如 s1: 'abcd'  s2: 'dcba'=> [3,2,1,0]
    2. 只按当前串中最大元素为线分割，分割成左右两个子串，两个子串的元素是连续的则可以（如， [1,3,2],  [7,6,9,10]），否则就不能还原成 连续的串
    3. 贪心，每次找最大的数分割 典型的二分发
 */


function Lc87(props) {

    /**
     * @param {string} s1
     * @param {string} s2
     * @return {boolean}
     */
    var isScramble = function(s1, s2) {

        // 特殊情况
        if (s1.length !== s2.length){
            return false;
        }

        if(s1.length === 1 ){
            return s1 === s2;
        }


        // 把s2转化为数组
        var toArr = function (sfrom,sto){

        }
        // 检查当前的数组元素是不是可连续的
        var chkContinue = function (arr){
            if(arr.length <= 1){
                return true;
            }

            let ta = JSON.parse(JSON.stringify(arr));
            ta.sort();
            return (ta[ta.length-1]-ta[0]) === (ta.length-1);
        }



        var sp = function (arr){

            // 判断当前串是否是连续的如果不连续则直接返回false
            if(!chkContinue(arr)){
                return false;
            }


            // 二分, 按最大的数字拆成左右
            let max = Math.max(arr);
            let left = [];
            let right = [];
            let fd = false;
            arr.forEach( (n) => {
                if (n === max){
                    fd = true;
                }else{
                    if (fd){
                        right.push(n);
                    }else{
                        left.push(n);
                    }
                }
            })

            return sp(left) && sp(right);
        }

        const ar = toArr(s1,s2);
        return sp(ar);
        // console.log(splitStr('abcde',3,true));
    };



    let s1='abcde';
    let s2='cdeba';


    return (
        <div>
            {isScramble(s1,s2)}
        </div>
    );
}

export default Lc87;
