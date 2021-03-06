package org.freebus.fts.products;

import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.FetchType;
import javax.persistence.GeneratedValue;
import javax.persistence.GenerationType;
import javax.persistence.Id;
import javax.persistence.JoinColumn;
import javax.persistence.Lob;
import javax.persistence.ManyToOne;
import javax.persistence.Table;
import javax.persistence.TableGenerator;

/**
 * Values of a parameter type.
 */
@Entity
@Table(name = "parameter_list_of_values")
public class ParameterValue
{
   @Id
   @TableGenerator(name = "ParameterValue", initialValue = 1, allocationSize = 100)
   @GeneratedValue(strategy = GenerationType.TABLE, generator = "ParameterValue")
   @Column(name = "parameter_value_id", nullable = false)
   private int id;

   @ManyToOne(optional = false, fetch = FetchType.LAZY)
   @JoinColumn(name = "parameter_type_id", nullable = false, referencedColumnName = "parameter_type_id")
   private ParameterType parameterType;

   @Column(name = "displayed_value")
   private String displayedValue;

   @Column(name = "display_order")
   private int displayOrder;

   @Column(name = "real_value")
   private int intValue;

   @Lob
   @Column(name = "binary_value")
   private byte[] binaryValue;

   @Column(name = "double_value")
   private double doubleValue;

   /**
    * @return the id
    */
   public int getId()
   {
      return id;
   }

   /**
    * @param id the id to set
    */
   public void setId(int id)
   {
      this.id = id;
   }

   /**
    * @return the parameter type to which this object belongs.
    */
   public ParameterType getParameterType()
   {
      return parameterType;
   }

   /**
    * Set the parameter type to which this object belongs.
    * 
    * @param parameterType - the parameter type to set.
    */
   public void setParameterType(ParameterType parameterType)
   {
      this.parameterType = parameterType;
   }

   /**
    * @return the displayedValue
    */
   public String getDisplayedValue()
   {
      return displayedValue;
   }

   /**
    * @param displayedValue the displayedValue to set
    */
   public void setDisplayedValue(String displayedValue)
   {
      this.displayedValue = displayedValue;
   }

   /**
    * @return the displayOrder
    */
   public int getDisplayOrder()
   {
      return displayOrder;
   }

   /**
    * @param displayOrder the displayOrder to set
    */
   public void setDisplayOrder(int displayOrder)
   {
      this.displayOrder = displayOrder;
   }

   /**
    * @return The integer value that will be loaded into the BCU's memory.
    */
   public int getIntValue()
   {
      return intValue;
   }

   /**
    * Set the integer value that will be loaded into the BCU's memory.
    *
    * @param intValue the intValue to set
    */
   public void setIntValue(int intValue)
   {
      this.intValue = intValue;
   }

   /**
    * @return The binary value that will be loaded into the BCU's memory. May be
    *         null.
    */
   public byte[] getBinaryValue()
   {
      return binaryValue;
   }

   /**
    * Set the binary value that will be loaded into the BCU's memory. May be
    * null.
    *
    * @param binaryValue - the binary value to set
    */
   public void setBinaryValue(byte[] binaryValue)
   {
      this.binaryValue = binaryValue;
   }

   /**
    * @return The double value that will be loaded into the BCU's memory.
    */
   public double getDoubleValue()
   {
      return doubleValue;
   }

   /**
    * Set the double value that will be loaded into the BCU's memory.
    *
    * @param doubleValue - the double value to set.
    */
   public void setDoubleValue(double doubleValue)
   {
      this.doubleValue = doubleValue;
   }

   /**
    * {@inheritDoc}
    */
   @Override
   public int hashCode()
   {
      return id;
   }
}
