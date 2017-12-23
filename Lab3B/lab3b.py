#!/usr/bin/python
#NAME: Jonathan Chon, William Tan
#EMAIL: jonchon@gmail.com, willtan510@gmail.com
#ID: 104780881, 104770108

import sys
import os

num_inodes=0
exit2_flag = 0
lvl_format = ["BLOCK", "INDIRECT BLOCK", "DOUBLE INDIRECT BLOCK", "TRIPLE INDIRECT BLOCK"]
offset_amount = [0, 12, 268, 65804]

def block_error(begin, end, level, block, inode, offset):
    global exit2_flag
    if offset == -1:
        local_offset = offset_amount[level]
    else:
        local_offset = offset
    if block > 0 and block < begin:
        print("RESERVED {lvl} {blck} IN INODE {inod} AT OFFSET {off}".format(lvl=lvl_format[level], blck=block, inod=inode, off=local_offset))
        exit2_flag = 1
        return False
    if block < 0 or block > end -1:
        print("INVALID {lvl} {blck} IN INODE {inod} AT OFFSET {off}".format(lvl=lvl_format[level], blck=block, inod=inode, off=local_offset))
        exit2_flag = 1
        return False
    return True

def check_summary(csv_file):
    block_dict = {}
    global exit2_flag
    global num_inodes
    
    file = open(csv_file, "r")
    if not file:
        sys.stderr.write("Error - Could not open {file}\n".format(file=csv_file))
        exit(1)
        
    if os.path.getsize(csv_file) == 0:
        sys.stderr.write("Error - {file} is empty\n".format(file=csv_file))
        exit(1)

    for summary in file:
        entry = summary.split(",")

        if entry[0] == "SUPERBLOCK":
            num_blocks = int(entry[1])
            num_inodes = int(entry[2])
            block_size = int(entry[3])
            inode_size = int(entry[4])

        #Don't need much since single group    
        if entry[0] == "GROUP":
            block_first_inode = int(entry[8])
            free_inode = block_first_inode + (inode_size * num_inodes / block_size)
            
        if entry[0] == "BFREE":
            block_num = int(entry[1])
            block_dict[block_num] = "FREE"

        if entry[0] == "INODE":
            inode_num = int(entry[1])
            for ind in range (12, 27): #Block addresses
                block_num = int(entry[ind])
                if ind < 24:
                    level = 0
                else:
                    level = ind-23
                if block_num == 0:
                    continue
                if block_num not in block_dict:
                    if(block_error(free_inode, num_blocks, level, block_num, inode_num, -1)):
                        block_dict[block_num] = (inode_num, level)
                elif block_dict[block_num] == "FREE":
                    print("ALLOCATED BLOCK {block} ON FREELIST".format(block=block_num))
                    exit2_flag = 1
                elif block_dict[block_num] != "FREE":
                    print("DUPLICATE {lyr} {blck} IN INODE {inod} AT OFFSET {off}".format(lyr=lvl_format[block_dict[block_num][1]], blck=block_num, inod=block_dict[block_num][0], off=offset_amount[block_dict[block_num][1]]))
                    print("DUPLICATE {lyr} {blck} IN INODE {inod} AT OFFSET {off}".format(lyr=lvl_format[level], blck=block_num, inod=inode_num, off=offset_amount[level]))              
                    exit2_flag = 1
                    
        if entry[0] == "INDIRECT":
            inode_num = int(entry[1])
            level = int(entry[2])
            offset = int(entry[3])
            block_num = int(entry[5])
            if block_num in block_dict:
                if block_dict[block_num] == "FREE":
                    print("ALLOCATED BLOCK {blck} ON FREELIST".format(blck=block_num))
                    exit2_flag = 1
                else:
                    print("DUPLICATE {lyr} {blck} IN INODE {inod} AT OFFSET {off}".format(lyr=lvl_format[block_dict[block_num][1]], blck=block_num, inod=block_dict[block_num][0], off=offset_amount[block_dict[block_num][1]]))
                    print("DUPLICATE {lyr} {blck} IN INODE {inod} AT OFFSET {off}".format(lyr=lvl_format[level], blck=block_num, inod=inode_num, off=offset_amount[level]))
                    exit2_flag = 1
            if(block_error(free_inode, num_blocks, level, block_num, inode_num, offset)):
                block_dict[block_num] = (inode_num, level)

    for i in range (free_inode, num_blocks):
        if i not in block_dict:
            print("UNREFERENCED BLOCK {blck}".format(blck=i))
            exit2_flag = 1
            
def inode_audit(csv_file):
    global num_inodes
    file = open(csv_file, "r")
    if not file:
        sys.stderr.write("Error - Could not open {file}\n".format(file=csv_file))
        exit(1)
        
    if os.path.getsize(csv_file) == 0:
        sys.stderr.write("Error - {file} is empty\n".format(file=csv_file))
        exit(1)
        
    for summary in file:
        entry=summary.split(",")
        
        if entry[0] == "SUPERBLOCK":
            num_inodes=int(entry[2])   
        
        
    inode_allocated=[False]*(num_inodes+1)  ## 0 if unallocated, 1 if allocated
    global inode_free
    inode_free=[False]*(num_inodes+1)      ## 0 if free, 1 if not free  
    file.seek(0)
    for summary in file:
        entry=summary.split(",")  
                   
        if entry[0] == "IFREE":
            inode_free[int(entry[1])]=True
        if entry[0] == "INODE":
            inode_allocated[int(entry[1])]=True

    for x in range (1,num_inodes):
        if x not in {1,3,4,5,6,7,8,9,10}: ##only for non-reserved inodes (1,3,4,5,6,7,8,9,10)
            if (inode_allocated[x] == True) and (inode_free[x] ==True):
                sys.stdout.write("ALLOCATED INODE {num} ON FREELIST\n".format(num=x))
            if (inode_free[x]==False) and (inode_allocated[x]==False):
                sys.stdout.write("UNALLOCATED INODE {num} NOT ON FREELIST\n".format(num=x))
                
def directory_audit(csv_file):
    global num_inodes
    global inode_free
    
    file = open(csv_file, "r")
    if not file:
        sys.stderr.write("Error - Could not open {file}\n".format(file=csv_file))
        exit(1)
        
    if os.path.getsize(csv_file) == 0:
        sys.stderr.write("Error - {file} is empty\n".format(file=csv_file))
        exit(1)
    
    directory_listLinks=[0]*(num_inodes+1)  ##inodeNum->linkCount
    inode_dirMap=[0]*(num_inodes+1)
    inode_dirCount=[0]*(num_inodes+1)
    inode_parentDir=[0]*(num_inodes+1)
    for summary in file:
        entry=summary.split(",")

        if entry[0]=="INODE" and entry[2]=="d":
            node_number=int(entry[1])
            if (node_number<1 or node_number>num_inodes):
                continue
            directory_listLinks[int(entry[1])]=int(entry[6])
            inode_dirMap[int(entry[1])]=1
            
        if entry[0]=="DIRENT":
            node_number=int(entry[3])
            if node_number<1 or node_number>num_inodes:
                sys.stdout.write("DIRECTORY INODE {inode} NAME {name} INVALID INODE {num}\n".format(inode=int(entry[1]),name=entry[6][:-1],num=int(entry[3])))  
                continue   
            if inode_free[node_number]==True and entry[6][:-1]!="'.'" and entry[6][:-1]!="'..'":
                sys.stdout.write("DIRECTORY INODE {inode} NAME {name} UNALLOCATED INODE {num}\n".format(inode=int(entry[1]),name=entry[6][:-1],num=int(entry[3])))                         
            if entry[6][:-1]!="'.'" and entry[6][:-1]!="'..'": 
                inode_parentDir[int(entry[3])]=int(entry[1])
 
    inode_parentDir[2]=2
            
    file.seek(0)
    for summary in file:
        entry=summary.split(",")
        if entry[0]=="INODE":
            if inode_parentDir[int(entry[1])]==0:
                sys.stdout.write("INODE {inode} HAS 0 LINKS BUT LINKCOUNT IS {num}\n".format(inode=int(entry[1]),num=int(entry[6])))
        if entry[0]=="DIRENT":  
            if int(entry[3])>num_inodes:
                continue
            if inode_dirMap[int(entry[3])]==1:
                inode_dirCount[int(entry[3])]+=1
            if entry[6][:-1]=="'.'":
                if int(entry[1])!=int(entry[3]):
                    sys.stdout.write("DIRECTORY INODE {num} NAME {name} LINK TO INODE {wrongNum} SHOULD BE {num}\n".format(num=int(entry[1]),name=entry[6][:-1],wrongNum=int(entry[3])))
            if entry[6][:-1]=="'..'":
                if int(entry[3])!=inode_parentDir[int(entry[1])]:
                    sys.stdout.write("DIRECTORY INODE {num} NAME {name} LINK TO INODE {wrongNum} SHOULD BE {rightNum}\n".format(num=int(entry[1]),name=entry[6][:-1],wrongNum=int(entry[3]),rightNum=inode_parentDir[int(entry[1])]))
    for i in range(1,num_inodes):
        if directory_listLinks[i]!=inode_dirCount[i]:
            sys.stdout.write("INODE {inode} HAS {ref_links} LINKS BUT LINKCOUNT IS {linkcount}\n".format(inode=i,ref_links=inode_dirCount[i],linkcount=directory_listLinks[i]))

            #get all inodes, classify if they're directory or file, if directory, look at the following dirents' parents, ++ inode[parent] for each one, see if matches linkcount for the parent
            

            
def main():
    global exit2_flag
    if len(sys.argv) != 2:
        sys.stderr.write("Error - Usage: ./lab3b FILENAME\n")
        exit(1)

    csv_file = sys.argv[1]
    if not os.path.isfile(csv_file):
        sys.stderr.write("Error - {file} does not exist in current directory\n".format(file=csv_file))
        exit(1)

    check_summary(csv_file)
    inode_audit(csv_file)
    directory_audit(csv_file)
    if exit2_flag:
        exit(2)
    exit(0)
if __name__ == '__main__':
    main()
