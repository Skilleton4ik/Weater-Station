import sys

def ascii_to_gyver(name, ascii_art):
    lines = [line.ljust(16)[:16] for line in ascii_art.strip('\n').split('\n')]
    while len(lines) < 16:
        lines.append(' '*16)
        
    pixels = [[1 if c in '#*' else 0 for c in line] for line in lines]
    
    gyver_bytes = []
    for page in range(2):
        for c in range(16):
            val = 0
            for r in range(8):
                val |= (pixels[page*8 + r][c] << r)
            gyver_bytes.append(val)
            
    out = f"static const uint8_t PROGMEM {name}[] = {{\n    "
    for i in range(32):
        out += f"0x{gyver_bytes[i]:02X},"
        if i % 8 == 7: out += " "
        if i % 16 == 15 and i != 31: out += "\n    "
    out = out.rstrip(', ') + "\n};\n"
    return out

icons = {}

icons['ICO_CLEAR'] = """
   *        *   
    *      *    
      ****      
*    ******    *
 *  ********  * 
   **********   
   **********   
 *  ********  * 
*    ******    *
      ****      
    *      *    
   *        *   
"""

icons['ICO_MOON'] = """
       ****     
     *******    
    ***   ***   
   **       *   
   *            
  **            
  **            
  **            
   *            
   **       *   
    ***   ***   
     *******    
       ****     
"""

icons['ICO_PARTLY'] = """
      ****      
*    ******     
 *  ********  * 
   ****   ***   
   **       **  
  **         ** 
  *           * 
  *           * 
  **         ** 
   ***********  
"""

icons['ICO_CLOUD'] = """
                
      *****     
    ***   ***   
   **       **  
  **         ** 
 **           **
 *             *
 *             *
 **           **
  ************* 
"""

icons['ICO_RAIN'] = """
      *****     
    ***   ***   
   **       **  
  **         ** 
 **           **
 *             *
 *             *
  ************* 
    *   *   *   
   *   *   *    
  *   *   *     
"""

icons['ICO_SNOW'] = """
      *****     
    ***   ***   
   **       **  
  **         ** 
 **           **
 *             *
 *             *
  ************* 
    *   *   *   
      *   *     
    *   *   *   
"""

icons['ICO_STORM'] = """
      *****     
    ***   ***   
   **       **  
  **         ** 
 **           **
 *             *
 *             *
  ************* 
      **        
     ***        
      **        
       *        
"""

icons['ICO_FOG'] = """
                
                
  ************* 
                
 ***************
                
  ************* 
                
 ***************
                
  ************* 
"""

for name, art in icons.items():
    print(ascii_to_gyver(name, art))
